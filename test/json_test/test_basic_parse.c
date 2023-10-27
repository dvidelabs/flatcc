#include <stdio.h>
#include "flatcc/flatcc_builder.h"
#include "flatcc/flatcc_json_parser.h"

/*
 * Helper macros for generating compile time tries.
 *
 * - this is for prototyping - codegenerator does this without macroes.
 */
#define __FLATCC_CHARW(x, p) (((uint64_t)(x)) << ((p) * 8))
#define __FLATCC_KW1(s) (__FLATCC_CHARW(s[0], 7))
#define __FLATCC_KW2(s) (__FLATCC_KW1(s) | __FLATCC_CHARW(s[1], 6))
#define __FLATCC_KW3(s) (__FLATCC_KW2(s) | __FLATCC_CHARW(s[2], 5))
#define __FLATCC_KW4(s) (__FLATCC_KW3(s) | __FLATCC_CHARW(s[3], 4))
#define __FLATCC_KW5(s) (__FLATCC_KW4(s) | __FLATCC_CHARW(s[4], 3))
#define __FLATCC_KW6(s) (__FLATCC_KW5(s) | __FLATCC_CHARW(s[5], 2))
#define __FLATCC_KW7(s) (__FLATCC_KW6(s) | __FLATCC_CHARW(s[6], 1))
#define __FLATCC_KW8(s) (__FLATCC_KW7(s) | __FLATCC_CHARW(s[7], 0))
#define __FLATCC_KW(s, n) __FLATCC_KW ## n(s)

#define __FLATCC_MASKKW(n) ((~(uint64_t)0) << ((8 - (n)) * 8))
#define __FLATCC_MATCHKW(w, s, n) ((__FLATCC_MASKKW(n) & (w)) == __FLATCC_KW(s, n))
#define __FLATCC_LTKW(w, s, n) ((__FLATCC_MASKKW(n) & (w)) < __FLATCC_KW(s, n))


const char g_data[] = "                                                     \
                                                                            \
{                               \r\n                                        \
    \"first\": 1,                                                           \
    \"second\": 2.0,                                                        \
    \"seconds left\": 42,                                                   \
    \"seconds lead\": 1,          \n                                        \
    \"zulu\": \"really\"      \n                                            \
}                                                                           \
";

/*
 * This is proof of concept test before code-generation to evaluate
 * efficient parsing and buffer construction principles while scanning
 * text such as a JSON. We do no use a schema per se, but implicitly
 * define one in the way that we construct the parser.
 */

#define match(x) if (end > buf && buf[0] == x) { ++buf; }                   \
        else { fprintf(stderr, "failed to match '%c'\n", x);                \
            buf = flatcc_json_parser_set_error(ctx, buf, end,              \
                    flatcc_json_parser_error_invalid_character); goto fail; }

/* Space is optional, but we do expect more input. */
#define space() {                                                           \
        buf = flatcc_json_parser_space(ctx, buf, end);                     \
        if (buf == end) { fprintf(stderr, "parse failed\n"); goto fail; }}  \

#ifdef FLATCC_JSON_ALLOW_UNKNOWN_FIELD
#define ignore_field() {                                                    \
    buf = flatcc_json_parser_symbol_end(ctx, buf, end);                    \
    space(); match(':'); space();                                           \
    buf = flatcc_json_parser_generic_json(ctx, buf, end);                  \
    if (buf == end) {                                                       \
        goto fail;                                                          \
    }}
#else
#define ignore_field() {                                                    \
    buf = flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_unknown_symbol);\
    goto fail; }
#endif


/*
 * We build a flatbuffer dynamically without a schema, but we still need
 * to assigned vtable entries.
 */
enum {
    id_first = 0,
    id_second = 1,
    id_seconds_left = 2,
    id_seconds_lead = 3,
    id_zulu = 10
};

enum {
    ctx_done = 0, ctx_t1_start, ctx_t1_again
};

const char *test(flatcc_builder_t *B, const char *buf, const char *end, int *ret)
{
    flatcc_json_parser_t parse_ctx, *ctx;
    flatcc_builder_ref_t root = 0, ref, *p_ref;
    uint64_t w;
    const char *k;
    char *s;
    flatcc_json_parser_escape_buffer_t code;

    void *p;

    ctx = &parse_ctx;
    memset(ctx, 0, sizeof(*ctx));
    ctx->line = 1;
    ctx->line_start = buf;

    flatcc_builder_start_buffer(B, "TEST", 0, 0);

    space(); match('{'); space();
    flatcc_builder_start_table(B, id_zulu + 1);

t1_again:

    buf = flatcc_json_parser_symbol_start(ctx, buf, end);
    w = flatcc_json_parser_symbol_part(buf, end);
    k = end - buf > 8 ? buf + 8 : end;
    /*
     * We implement a trie here. Because we compare big endian
     * any trailing garbage in a word is least significant
     * and masked out in MATCH tests.
     *
     * When a keyword is a prefix of another, the shorter keyword
     * must be tested first because any trailing "garbage" will
     * be larger (or equal if at buffer end or invalid nulls are
     * contained) than the short keyword, but if testing the long
     * keyword, the shorter keyword may be either larger or smaller
     * depending on what content follows.
     *
     * Errors result in `buf` being set to `end` so we need not test
     * for errors all the time. We use space as a convenient bailout
     * point.
     */
    if (__FLATCC_LTKW(w, "second", 6)) {
        if (!__FLATCC_MATCHKW(w, "first", 5)) {
            ignore_field();
        } else {
            buf = flatcc_json_parser_symbol_end(ctx, buf + 5, end);
            space(); match(':'); space();
            p = flatcc_builder_table_add(B, id_first, 1, 1);
            if (!p) { goto fail; }
            k = buf;
            buf = flatcc_json_parser_uint8(ctx, buf, end, p);
            /* Here we could optionally parse for symbolic constants. */
            if (k == buf) { goto fail; };
            /* Successfully parsed field. */
        }
    } else {
        if (__FLATCC_LTKW(w, "zulu", 4)) {
            if (__FLATCC_LTKW(w, "seconds ", 8)) {
                if (!__FLATCC_MATCHKW(w, "second", 6)) {
                    ignore_field();
                } else {
                    buf = flatcc_json_parser_symbol_end(ctx, buf + 6, end);
                    space(); match(':'); space();
                    p = flatcc_builder_table_add(B, id_second, 8, 8);
                    if (!p) { goto fail; }
                    k = buf;
                    buf = flatcc_json_parser_double(ctx, buf, end, p);
                    /* Here we could optionally parse for symbolic constants. */
                    if (k == buf) { goto fail; };
                    /* Successfully parsed field. */
                }
            } else {
                if (!__FLATCC_MATCHKW(w, "seconds ", 8)) {
                    ignore_field();
                } else {
                    /* We have multiple keys matching the first word, so we load another. */
                    buf = k;
                    w = flatcc_json_parser_symbol_part(buf, end);
                    k = end - buf > 8 ? buf + 8 : end;
                    if (__FLATCC_LTKW(w, "left", 4)) {
                        if (!__FLATCC_MATCHKW(w, "lead", 4)) {
                            ignore_field();
                        } else {
                            buf = flatcc_json_parser_symbol_end(ctx, buf + 4, end);
                            space(); match(':'); space();
                            p = flatcc_builder_table_add(B, id_seconds_lead, 8, 8);
                            if (!p) { goto fail; }
                            k = buf;
                            buf = flatcc_json_parser_int64(ctx, buf, end, p);
                            /* Here we could optionally parse for symbolic constants. */
                            if (k == buf) { goto fail; };
                            /* Successfully parsed field. */
                        }
                    } else {
                        if (!__FLATCC_MATCHKW(w, "left", 4)) {
                            ignore_field();
                        } else {
                            buf = flatcc_json_parser_symbol_end(ctx, buf + 4, end);
                            space(); match(':'); space();
                            p = flatcc_builder_table_add(B, id_seconds_left, 4, 4);
                            if (!p) { goto fail; }
                            k = buf;
                            buf = flatcc_json_parser_uint32(ctx, buf, end, p);
                            /* Here we could optionally parse for symbolic constants. */
                            if (k == buf) { goto fail; };
                            /* Successfully parsed field. */
                        }
                    }
                }
            }
        } else {
            if (!__FLATCC_MATCHKW(w, "zulu", 4)) {
                ignore_field();
            } else {
                buf = flatcc_json_parser_symbol_end(ctx, buf + 4, end);
                space(); match(':'); space();
                /*
                 * Parse field as string. If we are lucky, we can
                 * create the string in one go, which is faster.
                 * We can't if the string contains escape codes.
                 */
                buf = flatcc_json_parser_string_start(ctx, buf, end);
                k = buf;
                buf = flatcc_json_parser_string_part(ctx, buf, end);
                if (buf == end) {
                    goto fail;
                }
                if (buf[0] == '\"') {
                    ref = flatcc_builder_create_string(B, k, (size_t)(buf - k));
                } else {
                    /* Start string with enough space for what we have. */
                    flatcc_builder_start_string(B);
                    s = flatcc_builder_extend_string(B, (size_t)(buf - k));
                    if (!s) { goto fail; }
                    memcpy(s, k, (size_t)(buf - k));
                    do {
                        buf = flatcc_json_parser_string_escape(ctx, buf, end, code);
                        flatcc_builder_append_string(B, code + 1, (size_t)code[0]);
                        k = buf;
                        buf = flatcc_json_parser_string_part(ctx, buf, end);
                        if (buf == end) {
                            goto fail;
                        }
                        flatcc_builder_append_string(B, k, (size_t)(buf - k));
                    } while (buf[0] != '\"');
                    ref = flatcc_builder_end_string(B);
                }
                if (!ref) {
                    goto fail;
                }
                /* Duplicate fields may fail or assert. */
                p_ref = flatcc_builder_table_add_offset(B, id_zulu);
                if (!p_ref) {
                    goto fail;
                }
                *p_ref = ref;
                buf = flatcc_json_parser_string_end(ctx, buf, end);
                /* Successfully parsed field. */
            }
        }
    }
    space();
    if (*buf == ',') {
        ++buf;
        space();
        if (*buf != '}') {
            goto t1_again;
        }
#if !FLATCC_JSON_PARSE_ALLOW_TRAILING_COMMA
        return flatcc_json_parser_set_error(ctx, buf, end, flatcc_json_parser_error_trailing_comma);
#endif
    }
    match('}');
    root = flatcc_builder_end_table(B);

    flatcc_builder_end_buffer(B, root);
#if !FLATCC_JSON_PARSE_IGNORE_TRAILING_DATA
    buf = flatcc_json_parser_space(ctx, buf, end);
    if (buf != end) {
        fprintf(stderr, "extra characters in input\n");
        goto fail;
    }
#endif
fail:
    if (ctx->error) {
        fprintf(stderr, "%d:%d: %s\n", (int)ctx->line, (int)(ctx->error_loc - ctx->line_start + 1), flatcc_json_parser_error_string(ctx->error));
        flatcc_builder_reset(B);
    } else {
        fprintf(stderr, "parse accepted\n");
    }
    *ret = ctx->error;
    return buf;
}

int main(void)
{
    int ret = -1;
    flatcc_builder_t builder;

    flatcc_builder_init(&builder);

    test(&builder, g_data, g_data + sizeof(g_data) - 1, &ret);

    flatcc_builder_clear(&builder);
    return ret;
}
