#include "codegen_c.h"
#include "fileio.h"
#include <ctype.h>
#include "../../external/hash/str_set.h"

int fb_open_output_file(output_t *out, const char *name, int len, const char *ext)
{
    char *path;
    int ret;
    const char *prefix = out->opts->outpath ? out->opts->outpath : "";
    int prefix_len = strlen(prefix);

    if (out->opts->gen_stdout) {
        out->fp = stdout;
        return 0;
    }
    checkmem((path = fb_create_join_path(prefix, prefix_len, name, len, ext, 1)));
    out->fp = fopen(path, "wb");
    ret = 0;
    if (!out->fp) {
        fprintf(stderr, "error opening file for write: %s\n", path);
        ret = -1;
    }
    free(path);
    return ret;
}

void fb_close_output_file(output_t *out)
{
    if (out->fp != stdout && out->fp) {
        fclose(out->fp);
    }
    out->fp = 0;
}

int fb_init_output(output_t *out, fb_options_t *opts)
{
    const char *nsc;
    char *p;
    int n;

    memset(out, 0, sizeof(*out));
    out->opts = opts;
    nsc = opts->nsc;
    if (nsc) {
        n = strlen(opts->nsc);
        if (n > FLATCC_NAMESPACE_MAX) {
            fprintf(stderr, "common namespace argument is limited to %i characters\n", (int)FLATCC_NAMESPACE_MAX);
            return -1;
        }
    } else {
        nsc = FLATCC_DEFAULT_NAMESPACE_COMMON;
        n = strlen(nsc);
    }
    strncpy(out->nsc, nsc, FLATCC_NAMESPACE_MAX);
    out->nsc[FLATCC_NAMESPACE_MAX] = '\0';
    if (n) {
        out->nsc[n] = '_';
        out->nsc[n + 1] = '\0';
    }
    strcpy(out->nscup, out->nsc);
    for (p = out->nscup; *p; ++p) {
        *p = toupper(*p);
    }
    if (p != out->nscup) {
      p[-1] = '\0'; /* No trailing _ */
    }
    out->spacing = opts->cgen_spacing;
    return 0;
}

static void _str_set_destructor(void *context, char *item)
{
    (void)context;

    free(item);
}

/*
 * Removal of duplicate inclusions is only for a cleaner output - it is
 * not stricly necessary because the preprocessor handles include
 * guards. The guards are required to deal with concatenated files
 * regardless unless we generate special code for concatenation.
 */
void fb_gen_c_includes(output_t *out, const char *ext, const char *extup)
{
    fb_include_t *inc = out->S->includes;
    char *basename, *basenameup, *s;
    str_set_t set;

    fb_clear(set);

    /* Don't include our own file. */
    str_set_insert_item(&set, fb_copy_path(out->S->basenameup), ht_keep);
    while (inc) {
        checkmem((basename = fb_create_basename(
                    inc->name.s.s, inc->name.s.len, out->opts->default_schema_ext)));
        inc = inc->link;
        checkmem((basenameup = fb_copy_path(basename)));
        s = basenameup;
        while (*s) {
            *s = toupper(*s);
            ++s;
        }
        if (str_set_insert_item(&set, basenameup, ht_keep)) {
            free(basenameup);
            free(basename);
            continue;
        }
        /* The include guard is needed when concatening output. */
        fprintf(out->fp,
            "#ifndef %s%s\n"
            "#include \"%s%s\"\n"
            "#endif\n",
            basenameup, extup, basename, ext);
        free(basename);
        /* `basenameup` stored in str_set. */
    }
    str_set_destroy(&set, _str_set_destructor, 0);
}

int fb_copy_scope(fb_scope_t *scope, char *buf)
{
    int n, len;
    fb_ref_t *name;

    len = scope->prefix.len;
    for (name = scope->name; name; name = name->link) {
        n = name->ident->len;
        len += n + 1;
    }
    if (len > FLATCC_NAMESPACE_MAX + 1) {
        buf[0] = '\0';
        return -1;
    }
    len = scope->prefix.len;
    memcpy(buf, scope->prefix.s, len);
    for (name = scope->name; name; name = name->link) {
        n = name->ident->len;
        memcpy(buf + len, name->ident->text, n);
        len += n + 1;
        buf[len - 1] = '_';
    }
    buf[len] = '\0';
    return len;
}

void fb_scoped_symbol_name(fb_scope_t *scope, fb_symbol_t *sym, fb_scoped_name_t *sn)
{
    fb_token_t *t = sym->ident;

    if (sn->scope != scope) {
        if (0 > (sn->scope_len = fb_copy_scope(scope, sn->text))) {
            sn->scope_len = 0;
            fprintf(stderr, "skipping too long namespace\n");
        }
    }
    sn->len = t->len;
    sn->total_len = sn->scope_len + sn->len;
    if (sn->total_len > FLATCC_NAME_BUFSIZ - 1) {
        fprintf(stderr, "warning: truncating identifier: %.*s\n", sn->len, t->text);
        sn->len = FLATCC_NAME_BUFSIZ - sn->scope_len - 1;
        sn->total_len = sn->scope_len + sn->len;
    }
    memcpy(sn->text + sn->scope_len, t->text, sn->len);
    sn->text[sn->total_len] = '\0';
}

int fb_codegen_c(fb_options_t *opts, fb_schema_t *S)
{
    output_t output, *out;
    int ret, basename_len;

    out = &output;
    if (fb_init_output(out, opts)) {
        return -1;
    }
    out->S = S;
    out->current_scope = fb_scope_table_find(&S->root_schema->scope_index, 0, 0);
    basename_len = strlen(out->S->basename);
    ret = 0;
    if (opts->cgen_reader) {
        if (fb_open_output_file(out, out->S->basename, basename_len, "_reader.h")) {
            return -1;
        }
        ret = fb_gen_c_reader(out);
        fb_close_output_file(out);
    }
    if (!ret && opts->cgen_builder) {
        if (fb_open_output_file(out, out->S->basename, basename_len, "_builder.h")) {
            return -1;
        }
        ret = fb_gen_c_builder(out);
        fb_close_output_file(out);
    }
    if (!ret && opts->cgen_verifier) {
        if (fb_open_output_file(out, out->S->basename, basename_len, "_verifier.h")) {
            return -1;
        }
        ret = fb_gen_c_verifier(out);
        fb_close_output_file(out);
    }
    if (!ret && opts->cgen_json_parser) {
        if (fb_open_output_file(out, out->S->basename, basename_len, "_json_parser.h")) {
            return -1;
        }
        ret = fb_gen_c_json_parser(out);
        fb_close_output_file(out);
    }
    if (!ret && opts->cgen_json_printer) {
        if (fb_open_output_file(out, out->S->basename, basename_len, "_json_printer.h")) {
            return -1;
        }
        ret = fb_gen_c_json_printer(out);
        fb_close_output_file(out);
    }
    return ret;
}
