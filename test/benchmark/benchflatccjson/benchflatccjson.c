#define BENCH_TITLE "flatcc json parser and printer for C"

/*
 * NOTE:
 *
 * Using dtoa_grisu3.c over sprintf("%.17g") more than doubles the
 * encoding performance of this benchmark from 3.3 us/op to 1.3 us/op.
 */

#include <stdlib.h>

/*
 * Builder is only needed so we can create the initial buffer to encode
 * json from, but it also includes the reader which is needed calculate
 * the decoded checksum after parsing.
 */
#include "flatbench_builder.h"

#include "flatbench_json_parser.h"
#include "flatbench_json_printer.h"

#define C(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_FooBarContainer, x)
#define FooBar(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_FooBar, x)
#define Bar(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Bar, x)
#define Foo(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Foo, x)
#define Enum(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Enum, x)
#define True flatbuffers_true
#define False flatbuffers_false
#define StringLen flatbuffers_string_len

typedef struct flatcc_jsonbench {
    flatcc_builder_t builder;
    flatcc_json_parser_t parser;
    flatcc_json_printer_t printer;

    /* Holds the source data to print (encode) from. */
    char bin[1000];
    size_t bin_size;
    /* Extra buffer for extracting the parse (decoded) into. */
    char decode_buffer[1000];
    /*
     * The target encode / source decode buffer is provided by the
     * benchmark framework.
     */
} flatcc_jsonbench_t;

int flatcc_jsonbench_init(flatcc_jsonbench_t *bench)
{
    int i, veclen = 3;
    void *buffer_ok;
    flatcc_builder_t *B = &bench->builder;

    flatcc_builder_init(B);

    /* Generate the data needed to print from, just once. */
    C(start_as_root(B));
    C(list_start(B));
    for (i = 0; i < veclen; ++i) {
        /*
         * By using push_start instead of push_create we can construct
         * the sibling field (of Bar type) in-place on the stack,
         * otherwise we would need to create a temporary Bar struct.
         */
        C(list_push_start(B));
        FooBar(sibling_create(B,
                0xABADCAFEABADCAFE + i, 10000 + i, '@' + i, 1000000 + i,
                123456 + i, 3.14159f + i, 10000 + i));
        FooBar(name_create_str(B, "Hello, World!"));
        FooBar(rating_add(B, 3.1415432432445543543 + i));
        FooBar(postfix_add(B, '!' + i));
        C(list_push_end(B));
    }
    C(list_end(B));
    C(location_create_str(B, "https://www.example.com/myurl/"));
    C(fruit_add(B, Enum(Bananas)));
    C(initialized_add(B, True));
    C(end_as_root(B));

    buffer_ok = flatcc_builder_copy_buffer(B, bench->bin, sizeof(bench->bin));
    bench->bin_size = flatcc_builder_get_buffer_size(B);

    flatcc_builder_reset(&bench->builder);
    return !buffer_ok;
}

void flatcc_jsonbench_clear(flatcc_jsonbench_t *bench)
{
    flatcc_json_printer_clear(&bench->printer);
    flatcc_builder_clear(&bench->builder);
    // parser does not need to be cleared.
}

/*
 * For a buffer large enough to hold encoded representation.
 *
 * 1000 is enough for compact json, but for pretty printing we must up.
 */
#define BENCHMARK_BUFSIZ 10000

/* Interface to main benchmark logic. */
#define DECLARE_BENCHMARK(BM)                                               \
    flatcc_jsonbench_t flatcc_jsonbench, *BM;                               \
    BM = &flatcc_jsonbench;                                                 \
    flatcc_jsonbench_init(BM);

#define CLEAR_BENCHMARK(BM) flatcc_jsonbench_clear(BM);

int encode(flatcc_jsonbench_t *bench, void *buffer, size_t *size)
{
    int ret;

    flatcc_json_printer_init_buffer(&bench->printer, buffer, *size);
    /*
     * Normally avoid setting indentation - this yields compact
     * spaceless json which is what you want in resource critical
     * parsing and printing. But - it doesn't get that much slower,
     * so interesting to benchmark. Improve by enabling SSE4_2, but
     * generally not worth the trouble.
     */
    //flatcc_json_printer_set_indent(&bench->printer, 8);

    /*
     * Unquoted makes it slightly slower, noenum hardly makes a
     * difference - for this particular data set.
     */
    // flatcc_json_printer_set_noenum(&bench->printer, 1);
    // flatcc_json_printer_set_unquoted(&bench->printer, 1);
    ret = flatbench_print_json(&bench->printer, bench->bin, bench->bin_size);
    *size = flatcc_json_printer_flush(&bench->printer);

    return ret < 0 ? ret : 0;
}

int64_t decode(flatcc_jsonbench_t *bench, void *buffer, size_t size, int64_t sum)
{
    unsigned int i;
    int ret;
    flatcc_builder_t *B = &bench->builder;

    C(table_t) foobarcontainer;
    FooBar(vec_t) list;
    FooBar(table_t) foobar;
    Bar(struct_t) bar;
    Foo(struct_t) foo;

    flatcc_builder_reset(B);
    ret = flatbench_parse_json(B, &bench->parser, buffer, size, 0);
    if (ret) {
        return 0;
    }
    if (!flatcc_builder_copy_buffer(B,
            bench->decode_buffer, sizeof(bench->decode_buffer))) {
        return 0;
    }

    /* Traverse parsed result to calculate checksum. */

    foobarcontainer = C(as_root(bench->decode_buffer));
    sum += C(initialized(foobarcontainer));
    sum += StringLen(C(location(foobarcontainer)));
    sum += C(fruit(foobarcontainer));
    list = C(list(foobarcontainer));
    for (i = 0; i < FooBar(vec_len(list)); ++i) {
        foobar = FooBar(vec_at(list, i));
        sum += StringLen(FooBar(name(foobar)));
        sum += FooBar(postfix(foobar));
        sum += (int64_t)FooBar(rating(foobar));
        bar = FooBar(sibling(foobar));
        sum += (int64_t)Bar(ratio(bar));
        sum += Bar(size(bar));
        sum += Bar(time(bar));
        foo = Bar(parent(bar));
        sum += Foo(count(foo));
        sum += Foo(id(foo));
        sum += Foo(length(foo));
        sum += Foo(prefix(foo));
    }
    return sum + 2 * sum;
}

/* Copy to same folder before compilation or use include directive. */
#include "benchmain.h"
