#include "config.h"
#include "parser.h"
#include "semantics.h"
#include "fileio.h"
#include "codegen.h"
#include "codegen_c.h" /* for .depends */
#include "flatcc/flatcc.h"

void flatcc_init_options(flatcc_options_t *opts)
{
    memset(opts, 0, sizeof(*opts));

    opts->max_schema_size = FLATCC_MAX_SCHEMA_SIZE;
    opts->max_include_depth = FLATCC_MAX_INCLUDE_DEPTH;
    opts->max_include_count = FLATCC_MAX_INCLUDE_COUNT;
    opts->allow_boolean_conversion = FLATCC_ALLOW_BOOLEAN_CONVERSION;
    opts->allow_enum_key = FLATCC_ALLOW_ENUM_KEY;
    opts->allow_enum_struct_field = FLATCC_ALLOW_ENUM_STRUCT_FIELD;
    opts->allow_multiple_key_fields = FLATCC_ALLOW_MULTIPLE_KEY_FIELDS;
    opts->allow_string_key = FLATCC_ALLOW_STRING_KEY;
    opts->allow_struct_field_deprecate = FLATCC_ALLOW_STRUCT_FIELD_DEPRECATE;
    opts->allow_struct_field_key = FLATCC_ALLOW_STRUCT_FIELD_KEY;
    opts->allow_struct_root = FLATCC_ALLOW_STRUCT_ROOT;
    opts->ascending_enum = FLATCC_ASCENDING_ENUM;
    opts->hide_later_enum = FLATCC_HIDE_LATER_ENUM;
    opts->hide_later_struct = FLATCC_HIDE_LATER_STRUCT;
    opts->offset_size = FLATCC_OFFSET_SIZE;
    opts->voffset_size = FLATCC_VOFFSET_SIZE;
    opts->utype_size = FLATCC_UTYPE_SIZE;
    opts->bool_size = FLATCC_BOOL_SIZE;

    opts->require_root_type = FLATCC_REQUIRE_ROOT_TYPE;
    opts->strict_enum_init = FLATCC_STRICT_ENUM_INIT;
    /*
     * Index 0 is table elem count, and index 1 is table size
     * so max count is reduced by 2, meaning field id's
     * must be between 0 and vt_max_count - 1.
     * Usually, the table is 16-bit, so FLATCC_VOFFSET_SIZE = 2.
     * Strange expression to avoid shift overflow on 64 bit size.
     */
    opts->vt_max_count = ((1LL << (FLATCC_VOFFSET_SIZE * 8 - 1)) - 1) * 2;

    opts->default_schema_ext = FLATCC_DEFAULT_SCHEMA_EXT;
    opts->default_bin_schema_ext = FLATCC_DEFAULT_BIN_SCHEMA_EXT;
    opts->default_bin_ext = FLATCC_DEFAULT_BIN_EXT;

    opts->cgen_pad = FLATCC_CGEN_PAD;
    opts->cgen_sort = FLATCC_CGEN_SORT;
    opts->cgen_pragmas = FLATCC_CGEN_PRAGMAS;

    opts->cgen_common_reader = 0;
    opts->cgen_common_builder = 0;
    opts->cgen_reader = 0;
    opts->cgen_builder = 0;
    opts->cgen_json_parser = 0;
    opts->cgen_spacing = FLATCC_CGEN_SPACING;
    opts->cgen_depends = 0;

    opts->bgen_bfbs = FLATCC_BGEN_BFBS;
    opts->bgen_qualify_names = FLATCC_BGEN_QUALIFY_NAMES;
    opts->bgen_length_prefix = FLATCC_BGEN_LENGTH_PREFIX;
}

flatcc_context_t flatcc_create_context(flatcc_options_t *opts, const char *name,
        flatcc_error_fun error_out, void *error_ctx)
{
    fb_parser_t *P;

    if (!(P = malloc(sizeof(*P)))) {
        return 0;
    }
    if (fb_init_parser(P, opts, name, error_out, error_ctx, 0)) {
        free(P);
        return 0;
    }
    return P;
}

static flatcc_context_t __flatcc_create_child_context(flatcc_options_t *opts, const char *name,
        fb_parser_t *P_parent)
{
    fb_parser_t *P;

    if (!(P = malloc(sizeof(*P)))) {
        return 0;
    }
    if (fb_init_parser(P, opts, name, P_parent->error_out, P_parent->error_ctx, P_parent->schema.root_schema)) {
        free(P);
        return 0;
    }
    return P;
}

/* TODO: handle include files via some sort of buffer read callback
 * and possible transfer file based parser to this logic. */
int flatcc_parse_buffer(flatcc_context_t ctx, const char *buf, size_t buflen)
{
    fb_parser_t *P = ctx;

    /* Currently includes cannot be handled by buffers, so they should fail. */
    P->opts.disable_includes = 1;
    if ((size_t)buflen > P->opts.max_schema_size && P->opts.max_schema_size > 0) {
        fb_print_error(P, "input exceeds maximum allowed size\n");
        return -1;
    }
    /* Add self to set of visible schema. */
    ptr_set_insert_item(&P->schema.visible_schema, &P->schema, ht_keep);
    return fb_parse(P, buf, buflen, 0) || fb_build_schema(P) ? -1 : 0;
}

static void visit_dep(void *context, void *ptr)
{
    fb_schema_t *parent = context;
    fb_schema_t *dep = ptr;

    ptr_set_insert_item(&parent->visible_schema, dep, ht_keep);
}

static void add_visible_schema(fb_schema_t *parent, fb_schema_t *dep)
{
    ptr_set_visit(&dep->visible_schema, visit_dep, parent);
}

static int __parse_include_file(fb_parser_t *P_parent, const char *filename)
{
    flatcc_context_t *ctx = 0;
    fb_parser_t *P = 0;
    fb_root_schema_t *rs;
    flatcc_options_t *opts = &P_parent->opts;
    fb_schema_t *dep;

    rs = P_parent->schema.root_schema;
    if (rs->include_depth >= opts->max_include_depth && opts->max_include_depth > 0) {
        fb_print_error(P_parent, "include nesting level too deep\n");
        return -1;
    }
    if (rs->include_count >= opts->max_include_count && opts->max_include_count > 0) {
        fb_print_error(P_parent, "include count limit exceeded\n");
        return -1;
    }
    if (!(ctx = __flatcc_create_child_context(opts, filename, P_parent))) {
        return -1;
    }
    P = (fb_parser_t *)ctx;
    /* Don't parse the same file twice, or any other file with same name. */
    if ((dep = fb_schema_table_find_item(&rs->include_index, &P->schema))) {
        add_visible_schema(&P_parent->schema, dep);
        flatcc_destroy_context(ctx);
        return 0;
    }
    P->dependencies = P_parent->dependencies;
    P_parent->dependencies = P;
    P->referer_path = P_parent->path;
    /* Each parser has a root schema instance, but only the root parsers instance is used. */
    rs->include_depth++;
    rs->include_count++;
    if (flatcc_parse_file(ctx, filename)) {
        return -1;
    }
    add_visible_schema(&P_parent->schema, &P->schema);
    return 0;
}

/*
 * Not placed with code generators becuase our dependencies are stored
 * with the parser, not the schema.
 */
static int __flatcc_gen_depends_file(fb_parser_t *P)
{
    output_t output, *out;
    out = &output;

    /*
     * The dependencies list is only correct for root files as it is a
     * linear list. To deal with children, we would have to filter via
     * visible schema, but we don't really need that.
     */
    assert(P->referer_path == 0);

    if (fb_init_output(out, &P->opts)) {
        return -1;
    }
    out->S = &P->schema;
    if (fb_open_output_file(out, out->S->basename, strlen(out->S->basename), ".depends")) {
        return -1;
    }
    /* Don't depend on our selves. */
    P = P->dependencies;
    while (P) {
        size_t n = strlen(P->path);
        P->path[n] = '\n';
        fwrite(P->path, 1, n + 1, out->fp);
        P->path[n] = '\0';
        P = P->dependencies;
    }
    fb_close_output_file(out);
    return 0;
}

int flatcc_parse_file(flatcc_context_t ctx, const char *filename)
{
    fb_parser_t *P = ctx;
    size_t inpath_len, filename_len;
    char *buf, *path, *include_file;
    const char *inpath;
    size_t size;
    fb_name_t *inc;
    int i, ret;

    filename_len = strlen(filename);
    /* Don't parse the same file twice, or any other file with same basename. */
    if (fb_schema_table_insert_item(&P->schema.root_schema->include_index, &P->schema, ht_keep)) {
        return 0;
    }
    buf = 0;
    path = 0;
    include_file = 0;
    ret = -1;
    int is_root = !P->referer_path;

    /*
     * For root files, read file relative to working dir first. For
     * included files (`referer_path` set), first try include paths
     * in order, then path relative to including file.
     */
    if (is_root) {
        if (!(buf = fb_read_file(filename, P->opts.max_schema_size, &size))) {
            if (size + P->schema.root_schema->total_source_size > P->opts.max_schema_size && P->opts.max_schema_size > 0) {
                fb_print_error(P, "input exceeds maximum allowed size\n");
                ret = -1;
                goto fail;
            }
        } else {
            checkmem((path = fb_copy_path(filename)));
        }
    }
    for (i = 0; !buf && i < P->opts.inpath_count; ++i) {
        inpath = P->opts.inpaths[i];
        inpath_len = strlen(inpath);
        checkmem((path = fb_create_join_path(inpath, inpath_len, filename, filename_len, "", 1)));
        if (!(buf = fb_read_file(path, P->opts.max_schema_size, &size))) {
            free(path);
            path = 0;
            if (size > P->opts.max_schema_size && P->opts.max_schema_size > 0) {
                fb_print_error(P, "input exceeds maximum allowed size\n");
                ret = -1;
                goto fail;
            }
        }
    }
    if (!buf && !is_root) {
        inpath = P->referer_path;
        inpath_len = fb_find_basename(inpath, strlen(inpath));
        checkmem((path = fb_create_join_path(inpath, inpath_len, filename, filename_len, "", 1)));
        if (!(buf = fb_read_file(path, P->opts.max_schema_size, &size))) {
            free(path);
            path = 0;
            if (size > P->opts.max_schema_size && P->opts.max_schema_size > 0) {
                fb_print_error(P, "input exceeds maximum allowed size\n");
                ret = -1;
                goto fail;
            }
        }
    }
    if (!buf) {
        fb_print_error(P, "error reading included schema file: %s\n", filename);
        goto fail;
    }
    P->schema.root_schema->total_source_size += size;
    P->path = path;
    /* Parser owns path. */
    path = 0;
    /*
     * Even if we do not have the recursive option set, we still
     * need to parse all include files to make sense of the current
     * file.
     */
    if (!(ret = fb_parse(P, buf, size, 1))) {
        /* Parser owns buffer. */
        buf = 0;
        inc = P->schema.includes;
        while (inc) {
            checkmem((include_file = fb_copy_path_n(inc->name.s.s, inc->name.s.len)));
            if (__parse_include_file(P, include_file)) {
                goto fail;
            }
            free(include_file);
            include_file = 0;
            inc = inc->link;
        }
        /* Add self to set of visible schema. */
        ptr_set_insert_item(&P->schema.visible_schema, &P->schema, ht_keep);
        ret = fb_build_schema(P);
    }

    /*
     * We choose to only generate optional .depends files for root level
     * files. These will contain all nested files regardless of
     * recursive file generation flags.
     */
    if (P->opts.cgen_depends) {
        ret = __flatcc_gen_depends_file(P);
    }
fail:
    if (buf) {
        free(buf);
    }
    if (path) {
        free(path);
    }
    if (include_file) {
        free(include_file);
    }
    return ret;
}

#if FLATCC_REFLECTION
int flatcc_generate_binary_schema_to_buffer(flatcc_context_t ctx, void *buf, size_t bufsiz)
{
    fb_parser_t *P = ctx;

    if (fb_codegen_bfbs_to_buffer(&P->opts, &P->schema, buf, &bufsiz)) {
        return (int)bufsiz;
    }
    return -1;
}

void *flatcc_generate_binary_schema(flatcc_context_t ctx, size_t *size)
{
    fb_parser_t *P = ctx;

    return fb_codegen_bfbs_alloc_buffer(&P->opts, &P->schema, size);
}
#endif

int flatcc_generate_files(flatcc_context_t ctx)
{
    fb_parser_t *P = ctx, *P_leaf;
    int ret = 0;

    if (!P || P->failed) {
        return -1;
    }
    P_leaf = 0;
    while (P) {
        P->inverse_dependencies = P_leaf;
        P_leaf = P;
        P = P->dependencies;
    }
    P = ctx;
#if FLATCC_REFLECTION
    if (P->opts.bgen_bfbs) {
        if (fb_codegen_bfbs_to_file(&P->opts, &P->schema)) {
            return -1;
        }
    }
#endif
    /* This does not require a parse first. */
    if (fb_codegen_common_c(&P->opts)) {
        return -1;
    }
    /* If no file parsed - just common files if at all. */
    if (!P->has_schema) {
        return 0;
    }
    if (!P->opts.cgen_recursive) {
        return fb_codegen_c(&P->opts, &P->schema);
    }
    /* Make sure stdout output is generated in the right order. */
    P = P_leaf;
    while (!ret && P) {
        ret = P->failed || fb_codegen_c(&P->opts, &P->schema);
        P = P->inverse_dependencies;
    }
    return ret;
}

void flatcc_destroy_context(flatcc_context_t ctx)
{
    fb_parser_t *P = ctx, *dep = 0;

    while (P) {
        dep = P->dependencies;
        fb_clear_parser(P);
        free(P);
        P = dep;
    }
}
