/*
 * Runtime support for printing flatbuffers to JSON.
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "flatcc/flatcc_rtconfig.h"

/*
 * Grisu significantly improves printing speed of floating point values
 * and also the overall printing speed when floating point values are
 * present in non-trivial amounts. (Also applies to parsing).
 */
#if FLATCC_USE_GRISU3 && !defined(PORTABLE_USE_GRISU3)
#define PORTABLE_USE_GRISU3 1
#endif

#include "flatcc/flatcc_flatbuffers.h"
#include "flatcc/flatcc_json_printer.h"

#include "flatcc/portable/pprintint.h"
#include "flatcc/portable/pprintfp.h"

#define RAISE_ERROR(err) flatcc_json_printer_set_error(ctx, flatcc_json_printer_error_##err)

const char *flatcc_json_printer_error_string(int err)
{
    switch (err) {
#define XX(no, str)                                                         \
    case flatcc_json_printer_error_##no:                                    \
        return str;
        FLATCC_JSON_PRINT_ERROR_MAP(XX)
#undef XX
    default:
        return "unknown";
    }
}

#define uoffset_t flatbuffers_uoffset_t
#define soffset_t flatbuffers_soffset_t
#define voffset_t flatbuffers_voffset_t
#define utype_t flatbuffers_utype_t

#define uoffset_size sizeof(uoffset_t)
#define soffset_size sizeof(soffset_t)
#define voffset_size sizeof(voffset_t)
#define utype_size sizeof(utype_t)

#define offset_size uoffset_size

/* This hardcodes utype to uint8 so change if relevant. */
#define print_utype print_uint8

static inline const void *read_uoffset_ptr(const void *p)
{
    return (uint8_t *)p + __flatbuffers_uoffset_read_from_pe(p);
}

static inline const void *get_field_ptr(flatcc_json_printer_table_descriptor_t *td, int id)
{
    int vo = (id + 2) * sizeof(voffset_t);

    if (vo >= td->vsize) {
        return 0;
    }
    vo = *(voffset_t *)((uint8_t *)td->vtable + vo);
    if (vo == 0) {
        return 0;
    }
    return (uint8_t *)td->table + vo;
}

#define print_char(c) *ctx->p++ = (c)

#define print_start(c) do {                                                 \
    ++ctx->level;                                                           \
    *ctx->p++ = c;                                                          \
} while (0)

#define print_end(c) do {                                                   \
    if (ctx->indent) {                                                      \
        *ctx->p++ = '\n';                                                   \
        --ctx->level;                                                       \
        print_indent(ctx);                                                  \
    }                                                                       \
    *ctx->p++ = c;                                                          \
} while (0)

#define print_space() do {                                                  \
    *ctx->p = ' ';                                                          \
    ctx->p += !!ctx->indent;                                                \
} while (0)

#define print_nl() do {                                                     \
    if (ctx->indent) {                                                      \
        *ctx->p++ = '\n';                                                   \
        print_indent(ctx);                                                  \
    } else {                                                                \
        flatcc_json_printer_flush_partial(ctx);                             \
    }                                                                       \
} while (0)

/* Call at the end so print_end does not have to check for level. */
#define print_last_nl() do {                                                \
    if (ctx->indent && ctx->level == 0) {                                   \
        *ctx->p++ = '\n';                                                   \
    }                                                                       \
    flatcc_json_printer_flush_partial(ctx);                                 \
} while (0)

int flatcc_json_printer_fmt_float(char *buf, float n)
{
#if FLATCC_JSON_PRINT_HEX_FLOAT
    return print_hex_float(buf, n);
#else
    return print_float(n, buf);
#endif
}

int flatcc_json_printer_fmt_double(char *buf, double n)
{
#if FLATCC_JSON_PRINT_HEX_FLOAT
    return print_hex_double(buf, n);
#else
    return print_double(n, buf);
#endif
}

int flatcc_json_printer_fmt_bool(char *buf, int n)
{
    if (n) {
        memcpy(buf, "true", 4);
        return 4;
    }
    memcpy(buf, "false", 5);
    return 5;
}

static void print_string_part(flatcc_json_printer_t *ctx, const char *name, int len)
{
    size_t k;

    if (ctx->p + len >= ctx->pflush) {
        if (ctx->p >= ctx->pflush) {
            ctx->flush(ctx, 0);
        }
        do {
            k = ctx->pflush - ctx->p;
            memcpy(ctx->p, name, k);
            ctx->p += k;
            name += k;
            len -= k;
            ctx->flush(ctx, 0);
        } while (ctx->p + len >= ctx->pflush);
    }
    memcpy(ctx->p, name, len);
    ctx->p += len;
}

/*
 * Even though we know the the string length, we need to scan for escape
 * characters. There may be embedded zeroes. Beause FlatBuffer strings
 * are always zero terminated, we assume and optimize for this.
 *
 * We enforce \u00xx for control characters, but not for invalid
 * characters like 0xff - this makes it possible to handle some other
 * codepages transparently while formally not valid.  (Formally JSON
 * also supports UTF-16/32 little/big endian but flatbuffers only
 * support UTF-8 and we expect this in JSON input/output too).
 */
static void print_string(flatcc_json_printer_t *ctx, const char *name, size_t len)
{
    const char *p = name;
    /* Unsigned is important. */
    unsigned char c, x;
    size_t k;

    print_char('\"');
    for (;;) {
        /*
         */
        c = *p;
        while (c >= 0x20 && c != '\"' && c != '\\') {
            c = *++p;
        }
        k = p - name;
        print_string_part(ctx, name, k);
        len -= k;
        if (len == 0) {
            break;
        }
        name += k;
        print_char('\\');
        switch (c) {
        case '"': print_char('\"'); break;
        case '\\': print_char('\\'); break;
        case '\t' : print_char('t'); break;
        case '\f' : print_char('f'); break;
        case '\r' : print_char('r'); break;
        case '\n' : print_char('n'); break;
        case '\b' : print_char('b'); break;
        default:
            print_char('u');
            print_char('0');
            print_char('0');
            x = c >> 4;
            x += x < 10 ? '0' : 'a' - 10;
            print_char(x);
            x = c & 15;
            x += x < 10 ? '0' : 'a' - 10;
            print_char(x);
            break;
        }
        ++p;
        --len;
        ++name;
    }
    print_char('\"');
}

static void print_indent_ex(flatcc_json_printer_t *ctx, size_t k)
{
    size_t m;

    if (ctx->p >= ctx->pflush) {
        ctx->flush(ctx, 0);
    }
    m = ctx->pflush - ctx->p;
    while (k > m) {
        memset(ctx->p, ' ', m);
        ctx->p += m;
        k -= m;
        ctx->flush(ctx, 0);
        m = ctx->flush_size;
    }
    memset(ctx->p, ' ', k);
    ctx->p += k;
}

static inline void print_indent(flatcc_json_printer_t *ctx)
{
    size_t k = ctx->level * ctx->indent;

    if (ctx->p + k > ctx->pflush) {
        print_indent_ex(ctx, k);
    } else {
        memset(ctx->p, ' ', k);
        ctx->p += k;
    }
}

/*
 * Helpers for external use - does not do autmatic pretty printing, but
 * does escape strings.
 */
void flatcc_json_printer_string(flatcc_json_printer_t *ctx, const char *s, int n)
{
    print_string(ctx, s, n);
}
void flatcc_json_printer_write(flatcc_json_printer_t *ctx, const char *s, int n)
{
    print_string_part(ctx, s, n);
}
void flatcc_json_printer_nl(flatcc_json_printer_t *ctx)
{
    print_char('\n');
    flatcc_json_printer_flush_partial(ctx);
}
void flatcc_json_printer_char(flatcc_json_printer_t *ctx, char c)
{
    print_char(c);
}
void flatcc_json_printer_indent(flatcc_json_printer_t *ctx)
{
    /*
     * This is only needed when indent is 0 but helps external users
     * to avoid flushing when indenting.
     */
    print_indent(ctx);
}
void flatcc_json_printer_add_level(flatcc_json_printer_t *ctx, int n)
{
    ctx->level += n;
}
int flatcc_json_printer_get_level(flatcc_json_printer_t *ctx)
{
    return ctx->level;
}

static inline void print_type(flatcc_json_printer_t *ctx, const char *name, size_t len, utype_t type, const char *type_name, size_t type_len)
{
    print_nl();
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    if (ctx->p + len < ctx->pflush) {
        memcpy(ctx->p, name, len);
        ctx->p += len;
    } else {
        print_string_part(ctx, name, len);
    }
    print_string_part(ctx, "_type", 5);
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    print_char(':');
    print_space();
    if (ctx->noenum || type_name == 0) {
        ctx->p += print_utype(type, ctx->p);
    } else {
        *ctx->p = '\"';
        ctx->p += !ctx->unquote;
        print_string_part(ctx, type_name, type_len);
        *ctx->p = '\"';
        ctx->p += !ctx->unquote;
    }
}

static inline void print_symbol(flatcc_json_printer_t *ctx, const char *name, size_t len)
{
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
    if (ctx->p + len < ctx->pflush) {
        memcpy(ctx->p, name, len);
        ctx->p += len;
    } else {
        print_string_part(ctx, name, len);
    }
    *ctx->p = '\"';
    ctx->p += !ctx->unquote;
}

static inline void print_name(flatcc_json_printer_t *ctx, const char *name, size_t len)
{
    print_nl();
    print_symbol(ctx, name, len);
    print_char(':');
    print_space();
}

#define __flatcc_define_json_printer_scalar(TN, T)                          \
void flatcc_json_printer_ ## TN(                                            \
        flatcc_json_printer_t *ctx, T v)                                    \
{                                                                           \
    ctx->p += print_ ## TN(v, ctx->p);                                      \
}

__flatcc_define_json_printer_scalar(uint8, uint8_t)
__flatcc_define_json_printer_scalar(uint16, uint16_t)
__flatcc_define_json_printer_scalar(uint32, uint32_t)
__flatcc_define_json_printer_scalar(uint64, uint64_t)
__flatcc_define_json_printer_scalar(int8, int8_t)
__flatcc_define_json_printer_scalar(int16, int16_t)
__flatcc_define_json_printer_scalar(int32, int32_t)
__flatcc_define_json_printer_scalar(int64, int64_t)
__flatcc_define_json_printer_scalar(float, float)
__flatcc_define_json_printer_scalar(double, double)

void flatcc_json_printer_enum(flatcc_json_printer_t *ctx, const char *symbol, int len)
{
    print_symbol(ctx, symbol, len);
}

void flatcc_json_printer_delimit_enum_flags(flatcc_json_printer_t *ctx, int multiple)
{
#if FLATCC_JSON_PRINT_ALWAYS_QUOTE_MULTIPLE_FLAGS
    int quote = !ctx->unquote || multiple;
#else
    int quote = !ctx->unquote;
#endif
    *ctx->p = '"';
    ctx->p += quote;
}

void flatcc_json_printer_enum_flag(flatcc_json_printer_t *ctx, int count, const char *symbol, int len)
{
    *ctx->p = ' ';
    ctx->p += count > 0;
    print_string_part(ctx, symbol, len);
}

static inline void print_string_object(flatcc_json_printer_t *ctx, const void *p)
{
    size_t len;
    const char *s;

    len = (size_t)__flatbuffers_uoffset_read_from_pe(p);
    s = (const char *)p + uoffset_size;
    print_string(ctx, s, len);
}

#define __define_print_scalar_struct_field(TN, T)                           \
void flatcc_json_printer_ ## TN ## _struct_field(flatcc_json_printer_t *ctx,\
        int index, const void *p, size_t offset,                            \
        const char *name, int len)                                          \
{                                                                           \
    T x = flatbuffers_ ## TN ## _read_from_pe((uint8_t *)p + offset);       \
                                                                            \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    ctx->p += print_ ## TN (x, ctx->p);                                     \
}

#define __define_print_enum_struct_field(TN, T)                             \
void flatcc_json_printer_ ## TN ## _enum_struct_field(                      \
        flatcc_json_printer_t *ctx,                                         \
        int index, const void *p, size_t offset,                            \
        const char *name, int len,                                          \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x = flatbuffers_ ## TN ## _read_from_pe((uint8_t *)p + offset);       \
                                                                            \
    if (index) {                                                            \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    if (ctx->noenum) {                                                      \
        ctx->p += print_ ## TN (x, ctx->p);                                 \
    } else {                                                                \
        pf(ctx, x);                                                         \
    }                                                                       \
}

#define __define_print_scalar_field(TN, T)                                  \
void flatcc_json_printer_ ## TN ## _field(flatcc_json_printer_t *ctx,       \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, int len, T v)                             \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (p) {                                                                \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (x == v && ctx->skip_default) {                                  \
            return;                                                         \
        }                                                                   \
    } else {                                                                \
        if (!ctx->force_default) {                                          \
            return;                                                         \
        }                                                                   \
        x = v;                                                              \
    }                                                                       \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    ctx->p += print_ ## TN (x, ctx->p);                                     \
}

#define __define_print_enum_field(TN, T)                                    \
void flatcc_json_printer_ ## TN ## _enum_field(flatcc_json_printer_t *ctx,  \
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, int len, T v,                             \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    T x;                                                                    \
    const void *p = get_field_ptr(td, id);                                  \
                                                                            \
    if (p) {                                                                \
        x = flatbuffers_ ## TN ## _read_from_pe(p);                         \
        if (x == v && ctx->skip_default) {                                  \
            return;                                                         \
        }                                                                   \
    } else {                                                                \
        if (!ctx->force_default) {                                          \
            return;                                                         \
        }                                                                   \
        x = v;                                                              \
    }                                                                       \
    if (td->count++) {                                                      \
        print_char(',');                                                    \
    }                                                                       \
    print_name(ctx, name, len);                                             \
    if (ctx->noenum) {                                                      \
        ctx->p += print_ ## TN (x, ctx->p);                                 \
    } else {                                                                \
        pf(ctx, x);                                                         \
    }                                                                       \
}

static inline void print_table_object(flatcc_json_printer_t *ctx,
        const void *p, int ttl, flatcc_json_printer_table_f pf)
{
    flatcc_json_printer_table_descriptor_t td;

    if (!--ttl) {
        flatcc_json_printer_set_error(ctx, flatcc_json_printer_error_deep_recursion);
        return;
    }
    print_start('{');
    td.count = 0;
    td.ttl = ttl;
    td.table = p;
    td.vtable = (uint8_t *)p - __flatbuffers_soffset_read_from_pe(p);
    td.vsize = __flatbuffers_voffset_read_from_pe(td.vtable);
    pf(ctx, &td);
    print_end('}');
}

void flatcc_json_printer_string_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_string_object(ctx, read_uoffset_ptr(p));
    }
}

#define __define_print_scalar_vector_field(TN, T)                           \
void flatcc_json_printer_ ## TN ## _vector_field(flatcc_json_printer_t *ctx,\
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, int len)                                  \
{                                                                           \
    const void *p = get_field_ptr(td, id);                                  \
    uoffset_t count;                                                        \
                                                                            \
    if (p) {                                                                \
        if (td->count++) {                                                  \
            print_char(',');                                                \
        }                                                                   \
        p = read_uoffset_ptr(p);                                            \
        count = __flatbuffers_uoffset_read_from_pe(p);                      \
        p = (void *)((size_t)p + uoffset_size);                             \
        print_name(ctx, name, len);                                         \
        print_start('[');                                                   \
        if (count) {                                                        \
            print_nl();                                                     \
            ctx->p += print_ ## TN (                                        \
                    flatbuffers_ ## TN ## _read_from_pe(p),                 \
                    ctx->p);                                                \
            p = (void *)((size_t)p + sizeof(T));                            \
            --count;                                                        \
        }                                                                   \
        while (count--) {                                                   \
            print_char(',');                                                \
            print_nl();                                                     \
            ctx->p += print_ ## TN (                                        \
                    flatbuffers_ ## TN ## _read_from_pe(p),                 \
                    ctx->p);                                                \
            p = (void *)((size_t)p + sizeof(T));                            \
        }                                                                   \
        print_end(']');                                                     \
    }                                                                       \
}

#define __define_print_enum_vector_field(TN, T)                             \
void flatcc_json_printer_ ## TN ## _enum_vector_field(flatcc_json_printer_t *ctx,\
        flatcc_json_printer_table_descriptor_t *td,                         \
        int id, const char *name, int len,                                  \
        flatcc_json_printer_ ## TN ##_enum_f *pf)                           \
{                                                                           \
    const void *p;                                                          \
    uoffset_t count;                                                        \
                                                                            \
    if (ctx->noenum) {                                                      \
        flatcc_json_printer_ ## TN ## _vector_field(ctx, td, id, name, len);\
        return;                                                             \
    }                                                                       \
    p = get_field_ptr(td, id);                                              \
    if (p) {                                                                \
        if (td->count++) {                                                  \
            print_char(',');                                                \
        }                                                                   \
        p = read_uoffset_ptr(p);                                            \
        count = __flatbuffers_uoffset_read_from_pe(p);                      \
        p = (void *)((size_t)p + uoffset_size);                             \
        print_name(ctx, name, len);                                         \
        print_start('[');                                                   \
        if (count) {                                                        \
            print_nl();                                                     \
            pf(ctx, flatbuffers_ ## TN ## _read_from_pe(p));                \
            p = (void *)((size_t)p + sizeof(T));                            \
            --count;                                                        \
        }                                                                   \
        while (count--) {                                                   \
            print_char(',');                                                \
            print_nl();                                                     \
            pf(ctx, flatbuffers_ ## TN ## _read_from_pe(p));                \
            p = (void *)((size_t)p + sizeof(T));                            \
        }                                                                   \
        print_end(']');                                                     \
    }                                                                       \
}

__define_print_scalar_field(uint8, uint8_t)
__define_print_scalar_field(uint16, uint16_t)
__define_print_scalar_field(uint32, uint32_t)
__define_print_scalar_field(uint64, uint64_t)
__define_print_scalar_field(int8, int8_t)
__define_print_scalar_field(int16, int16_t)
__define_print_scalar_field(int32, int32_t)
__define_print_scalar_field(int64, int64_t)
__define_print_scalar_field(bool, flatbuffers_bool_t)
__define_print_scalar_field(float, float)
__define_print_scalar_field(double, double)

__define_print_enum_field(uint8, uint8_t)
__define_print_enum_field(uint16, uint16_t)
__define_print_enum_field(uint32, uint32_t)
__define_print_enum_field(uint64, uint64_t)
__define_print_enum_field(int8, int8_t)
__define_print_enum_field(int16, int16_t)
__define_print_enum_field(int32, int32_t)
__define_print_enum_field(int64, int64_t)
__define_print_enum_field(bool, flatbuffers_bool_t)

__define_print_scalar_struct_field(uint8, uint8_t)
__define_print_scalar_struct_field(uint16, uint16_t)
__define_print_scalar_struct_field(uint32, uint32_t)
__define_print_scalar_struct_field(uint64, uint64_t)
__define_print_scalar_struct_field(int8, int8_t)
__define_print_scalar_struct_field(int16, int16_t)
__define_print_scalar_struct_field(int32, int32_t)
__define_print_scalar_struct_field(int64, int64_t)
__define_print_scalar_struct_field(bool, flatbuffers_bool_t)
__define_print_scalar_struct_field(float, float)
__define_print_scalar_struct_field(double, double)

__define_print_enum_struct_field(uint8, uint8_t)
__define_print_enum_struct_field(uint16, uint16_t)
__define_print_enum_struct_field(uint32, uint32_t)
__define_print_enum_struct_field(uint64, uint64_t)
__define_print_enum_struct_field(int8, int8_t)
__define_print_enum_struct_field(int16, int16_t)
__define_print_enum_struct_field(int32, int32_t)
__define_print_enum_struct_field(int64, int64_t)
__define_print_enum_struct_field(bool, flatbuffers_bool_t)

__define_print_scalar_vector_field(uint8, uint8_t)
__define_print_scalar_vector_field(uint16, uint16_t)
__define_print_scalar_vector_field(uint32, uint32_t)
__define_print_scalar_vector_field(uint64, uint64_t)
__define_print_scalar_vector_field(int8, int8_t)
__define_print_scalar_vector_field(int16, int16_t)
__define_print_scalar_vector_field(int32, int32_t)
__define_print_scalar_vector_field(int64, int64_t)
__define_print_scalar_vector_field(bool, flatbuffers_bool_t)
__define_print_scalar_vector_field(float, float)
__define_print_scalar_vector_field(double, double)

__define_print_enum_vector_field(uint8, uint8_t)
__define_print_enum_vector_field(uint16, uint16_t)
__define_print_enum_vector_field(uint32, uint32_t)
__define_print_enum_vector_field(uint64, uint64_t)
__define_print_enum_vector_field(int8, int8_t)
__define_print_enum_vector_field(int16, int16_t)
__define_print_enum_vector_field(int32, int32_t)
__define_print_enum_vector_field(int64, int64_t)
__define_print_enum_vector_field(bool, flatbuffers_bool_t)

void flatcc_json_printer_struct_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        size_t size,
        flatcc_json_printer_struct_f pf)
{
    const uint8_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        p += uoffset_size;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_nl();
            print_start('{');
            pf(ctx, p);
            print_end('}');
            --count;
        }
        while (count--) {
            p += size;
            print_char(',');
            print_nl();
            print_start('{');
            pf(ctx, p);
            print_end('}');
        }
        print_end(']');
    }
}

void flatcc_json_printer_string_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len)
{
    const uoffset_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        ++p;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_nl();
            print_string_object(ctx, read_uoffset_ptr(p));
            --count;
        }
        while (count--) {
            ++p;
            print_char(',');
            print_nl();
            print_string_object(ctx, read_uoffset_ptr(p));
        }
        print_end(']');
    }
}

void flatcc_json_printer_table_vector_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        flatcc_json_printer_table_f pf)
{
    const uoffset_t *p = get_field_ptr(td, id);
    uoffset_t count;

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        p = read_uoffset_ptr(p);
        count = __flatbuffers_uoffset_read_from_pe(p);
        ++p;
        print_name(ctx, name, len);
        print_start('[');
        if (count) {
            print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
            --count;
        }
        while (count--) {
            ++p;
            print_char(',');
            print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
        }
        print_end(']');
    }
}

void flatcc_json_printer_table_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        flatcc_json_printer_table_f pf)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_table_object(ctx, read_uoffset_ptr(p), td->ttl, pf);
    }
}

void flatcc_json_printer_embedded_struct_field(flatcc_json_printer_t *ctx,
        int index, const void *p, size_t offset,
        const char *name, int len,
        flatcc_json_printer_struct_f pf)
{
    if (index) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_start('{');
    pf(ctx, (uint8_t *)p + offset);
    print_end('}');
}

void flatcc_json_printer_struct_field(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        flatcc_json_printer_struct_f *pf)
{
    const void *p = get_field_ptr(td, id);

    if (p) {
        if (td->count++) {
            print_char(',');
        }
        print_name(ctx, name, len);
        print_start('{');
        pf(ctx, p);
        print_end('}');
    }
}

/*
 * Make sure the buffer identifier is valid before assuming the rest of
 * the buffer is sane.
 */
static int accept_header(flatcc_json_printer_t * ctx,
        const void *buf, size_t bufsiz, const char *fid)
{
    if (buf == 0 || bufsiz < offset_size + FLATBUFFERS_IDENTIFIER_SIZE) {
        RAISE_ERROR(bad_input);
        assert(0 && "buffer header too small");
        return 0;
    }
    if (fid != 0 && 0 != memcmp((uint8_t *)buf + offset_size, fid, FLATBUFFERS_IDENTIFIER_SIZE)) {
        RAISE_ERROR(bad_input);
        assert(0 && "identifier mismatch");
        return 0;
    }
    return 1;
}

int flatcc_json_printer_struct_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid,
        flatcc_json_printer_struct_f *pf)
{
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return -1;
    }
    print_start('{');
    pf(ctx, read_uoffset_ptr(buf));
    print_end('}');
    print_last_nl();
    return flatcc_json_printer_get_error(ctx) ? -1 : (int)ctx->total;
}

int flatcc_json_printer_table_as_root(flatcc_json_printer_t *ctx,
        const void *buf, size_t bufsiz, const char *fid, flatcc_json_printer_table_f *pf)
{
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return -1;
    }
    print_table_object(ctx, read_uoffset_ptr(buf), FLATCC_JSON_PRINT_MAX_LEVELS, pf);
    print_last_nl();
    return flatcc_json_printer_get_error(ctx) ? -1 : (int)ctx->total;
}

void flatcc_json_printer_struct_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        const char *fid,
        flatcc_json_printer_struct_f *pf)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    if (0 == (buf = get_field_ptr(td, id))) {
        return;
    }
    buf = (const uoffset_t *)((size_t)buf + __flatbuffers_uoffset_read_from_pe(buf));
    bufsiz = __flatbuffers_uoffset_read_from_pe(buf);
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return;
    }
    if (td->count++) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_start('{');
    pf(ctx, read_uoffset_ptr(buf));
    print_end('}');
}

void flatcc_json_printer_table_as_nested_root(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        int id, const char *name, int len,
        const char *fid,
        flatcc_json_printer_table_f pf)
{
    const uoffset_t *buf;
    uoffset_t bufsiz;

    if (0 == (buf = get_field_ptr(td, id))) {
        return;
    }
    buf = (const uoffset_t *)((size_t)buf + __flatbuffers_uoffset_read_from_pe(buf));
    bufsiz = __flatbuffers_uoffset_read_from_pe(buf);
    ++buf;
    if (!accept_header(ctx, buf, bufsiz, fid)) {
        return;
    }
    if (td->count++) {
        print_char(',');
    }
    print_name(ctx, name, len);
    print_table_object(ctx, read_uoffset_ptr(buf), td->ttl, pf);
}

int flatcc_json_printer_read_union_type(
        flatcc_json_printer_table_descriptor_t *td,
        int id)
{
    const void *p = get_field_ptr(td, id - 1);

    int x = p ? __flatbuffers_utype_read_from_pe(p) : 0;
    return x;
}

void flatcc_json_printer_union_type(flatcc_json_printer_t *ctx,
        flatcc_json_printer_table_descriptor_t *td,
        const char *name, int len, int type,
        const char *type_name, int type_len)
{
    if (td->count++) {
        print_char(',');
    }
    print_type(ctx, name, len, type, type_name, type_len);
}

static void __flatcc_json_printer_flush(flatcc_json_printer_t *ctx, int all)
{
    if (!all && ctx->p >= ctx->pflush) {
        size_t spill = ctx->p - ctx->pflush;

        fwrite(ctx->buf, ctx->flush_size, 1, ctx->fp);
        memcpy(ctx->buf, ctx->buf + ctx->flush_size, spill);
        ctx->p = ctx->buf + spill;
        ctx->total += ctx->flush_size;
    } else {
        size_t len = ctx->p - ctx->buf;

        fwrite(ctx->buf, len, 1, ctx->fp);
        ctx->p = ctx->buf;
        ctx->total += len;
    }
}

int flatcc_json_printer_init(flatcc_json_printer_t *ctx, void *fp)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->fp = fp ? fp : stdout;
    ctx->flush = __flatcc_json_printer_flush;
    if (!(ctx->buf = malloc(FLATCC_JSON_PRINT_BUFFER_SIZE))) {
        return -1;
    }
    ctx->own_buffer = 1;
    ctx->size = FLATCC_JSON_PRINT_BUFFER_SIZE;
    ctx->flush_size = FLATCC_JSON_PRINT_FLUSH_SIZE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    /*
     * Make sure we have space for primitive operations such as printing numbers
     * without having to flush.
     */
    assert(ctx->flush_size + FLATCC_JSON_PRINT_RESERVE <= ctx->size);
    return 0;
}

static void __flatcc_json_printer_flush_buffer(flatcc_json_printer_t *ctx, int all)
{
    (void)all;

    ctx->total += ctx->p - ctx->buf;
    ctx->p = ctx->buf;
    RAISE_ERROR(overflow);
}

int flatcc_json_printer_init_buffer(flatcc_json_printer_t *ctx, char *buffer, size_t buffer_size)
{
    assert(buffer_size >= FLATCC_JSON_PRINT_RESERVE);
    if (buffer_size < FLATCC_JSON_PRINT_RESERVE) {
        return -1;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = buffer;
    ctx->size = buffer_size;
    ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    ctx->flush = __flatcc_json_printer_flush_buffer;
    return 0;
}

static void __flatcc_json_printer_flush_dynamic_buffer(flatcc_json_printer_t *ctx, int all)
{
    (void)all;

    size_t len = ctx->p - ctx->buf;

    char *p = realloc(ctx->buf, ctx->size * 2);

    ctx->total += len;
    if (!p) {
        RAISE_ERROR(overflow);
        ctx->p = ctx->buf;
    } else {
        ctx->size *= 2;
        ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
        ctx->buf = p;
        ctx->p = p + len;
        ctx->pflush = p + ctx->flush_size;
    }
}

int flatcc_json_printer_init_dynamic_buffer(flatcc_json_printer_t *ctx, size_t buffer_size)
{
    if (buffer_size == 0) {
        buffer_size = FLATCC_JSON_PRINT_DYN_BUFFER_SIZE;
    }
    if (buffer_size < FLATCC_JSON_PRINT_RESERVE) {
        buffer_size = FLATCC_JSON_PRINT_RESERVE;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf = malloc(buffer_size);
    ctx->own_buffer = 1;
    ctx->size = buffer_size;
    ctx->flush_size = ctx->size - FLATCC_JSON_PRINT_RESERVE;
    ctx->p = ctx->buf;
    ctx->pflush = ctx->buf + ctx->flush_size;
    ctx->flush = __flatcc_json_printer_flush_dynamic_buffer;
    if (!ctx->buf) {
        RAISE_ERROR(overflow);
        return -1;
    }
    return 0;
}

void *flatcc_json_printer_get_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size)
{
    if (buffer_size) {
        *buffer_size = ctx->p - ctx->buf;
    }
    *ctx->p = '\0';
    return ctx->buf;
}

void *flatcc_json_printer_finalize_dynamic_buffer(flatcc_json_printer_t *ctx, size_t *buffer_size)
{
    void *buffer;

    flatcc_json_printer_nl(ctx);
    flatcc_json_printer_flush(ctx);
    buffer = flatcc_json_printer_get_buffer(ctx, buffer_size);
    memset(ctx, 0, sizeof(*ctx));
    return buffer;
}

void flatcc_json_printer_clear(flatcc_json_printer_t *ctx)
{
    if (ctx->own_buffer && ctx->buf) {
        free(ctx->buf);
    }
    memset(ctx, 0, sizeof(*ctx));
}
