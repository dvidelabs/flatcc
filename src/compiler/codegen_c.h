#ifndef CODEGEN_C_H
#define CODEGEN_C_H

#include <assert.h>

#include "symbols.h"
#include "parser.h"
#include "codegen.h"

#define __FLATCC_ERROR_TYPE "INTERNAL_ERROR_UNEXPECTED_TYPE"

#ifndef gen_panic
#define gen_panic(context, msg) fprintf(stderr, "%s\n", msg), assert(0), exit(-1)
#endif

typedef struct output output_t;

struct output {
    /*
     * Common namespace across files. May differ from namespace
     * for consistent use of type names.
     */
    char nsc[FLATCC_NAMESPACE_MAX + 2];
    char nscup[FLATCC_NAMESPACE_MAX + 2];

    FILE *fp;
    fb_schema_t *S;
    fb_options_t *opts;
    fb_scope_t *current_scope;
};

static inline void token_name(fb_token_t *t, int *n, const char **s) {
    *n = t->len;
    *s = t->text;
}

typedef char fb_symbol_text_t[FLATCC_NAME_BUFSIZ];
typedef struct fb_scoped_name fb_scoped_name_t;

/* Should be zeroed because scope is cached across updates. */
struct fb_scoped_name {
    fb_symbol_text_t text;
    fb_scope_t *scope;
    int scope_len, len, total_len;
};

#define fb_clear(x) (memset(&(x), 0, sizeof(x)))

/* Returns length or -1 if length exceeds namespace max. */
int __flatcc_fb_copy_scope(fb_scope_t *scope, char *buf);
#define fb_copy_scope __flatcc_fb_copy_scope

void __flatcc_fb_scoped_symbol_name(fb_scope_t *scope, fb_symbol_t *sym, fb_scoped_name_t *sn);
#define fb_scoped_symbol_name __flatcc_fb_scoped_symbol_name

static inline void fb_compound_name(fb_compound_type_t *ct, fb_scoped_name_t *sn)
{
    fb_scoped_symbol_name(ct->scope, &ct->symbol, sn);
}

static inline void symbol_name(fb_symbol_t *sym, int *n, const char **s) {
    token_name(sym->ident, n, s);
}

static inline const char *scalar_type_ns(fb_scalar_type_t scalar_type, const char *ns)
{
    return scalar_type == fb_bool ? ns : "";
}

static inline const char *scalar_type_prefix(fb_scalar_type_t scalar_type)
{
    const char *tname;
    switch (scalar_type) {
    case fb_ulong:
        tname = "uint64";
        break;
    case fb_uint:
        tname = "uint32";
        break;
    case fb_ushort:
        tname = "uint16";
        break;
    case fb_ubyte:
        tname = "uint8";
        break;
    case fb_bool:
        tname = "bool";
        break;
    case fb_long:
        tname = "int64";
        break;
    case fb_int:
        tname = "int32";
        break;
    case fb_short:
        tname = "int16";
        break;
    case fb_byte:
        tname = "int8";
        break;
    case fb_float:
        tname = "float";
        break;
    case fb_double:
        tname = "double";
        break;
    default:
        gen_panic(out, "internal error: unexpected type during code generation");
        tname = __FLATCC_ERROR_TYPE;
        break;
    }
    return tname;
}

static inline const char *scalar_type_name(fb_scalar_type_t scalar_type)
{
    const char *tname;
    switch (scalar_type) {
    case fb_ulong:
        tname = "uint64_t";
        break;
    case fb_uint:
        tname = "uint32_t";
        break;
    case fb_ushort:
        tname = "uint16_t";
        break;
    case fb_ubyte:
        tname = "uint8_t";
        break;
    case fb_bool:
        tname = "bool_t";
        break;
    case fb_long:
        tname = "int64_t";
        break;
    case fb_int:
        tname = "int32_t";
        break;
    case fb_short:
        tname = "int16_t";
        break;
    case fb_byte:
        tname = "int8_t";
        break;
    case fb_float:
        tname = "float";
        break;
    case fb_double:
        tname = "double";
        break;
    default:
        gen_panic(out, "internal error: unexpected type during code generation");
        tname = __FLATCC_ERROR_TYPE;
        break;
    }
    return tname;
}

void __flatcc_fb_gen_c_includes(output_t *out, const char *ext, const char *extup);
#define fb_gen_c_includes __flatcc_fb_gen_c_includes

int __flatcc_fb_gen_common_c_header(output_t *out);
#define fb_gen_common_c_header __flatcc_fb_gen_common_c_header

int __flatcc_fb_gen_common_c_builder_header(output_t *out);
#define fb_gen_common_c_builder_header __flatcc_fb_gen_common_c_builder_header

int __flatcc_fb_gen_c_reader(output_t *out);
#define fb_gen_c_reader __flatcc_fb_gen_c_reader

int __flatcc_fb_gen_c_builder(output_t *out);
#define fb_gen_c_builder __flatcc_fb_gen_c_builder


static inline int gen_pragma_push(output_t *out)
{
    if (out->opts->cgen_pragmas) {
        fprintf(out->fp,
                "#if __clang__\n"
                "#pragma clang diagnostic push\n"
                "#pragma clang diagnostic ignored \"-Wunused-function\"\n"
                "#pragma clang diagnostic ignored \"-Wunused-variable\"\n"
                "#endif\n");
    }
    return 0;
}

static inline int gen_pragma_pop(output_t *out)
{
    if (out->opts->cgen_pragmas) {
        fprintf(out->fp,
                "#if __clang__\n"
                "#pragma clang diagnostic pop\n"
                "#endif\n");
    }
    return 0;
}

#endif /* CODEGEN_C_H */
