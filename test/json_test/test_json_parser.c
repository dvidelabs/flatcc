#include <stdio.h>

#ifndef FLATCC_BENCHMARK 
#define FLATCC_BENCHMARK 0
#endif

/* Only needed for verification. */
#include "monster_test_reader.h"
#include "monster_test_json_parser.h"
#include "flatcc/support/hexdump.h"
#include "flatcc/support/cdump.h"
#include "flatcc/support/readfile.h"

#if FLATCC_BENCHMARK
#include "flatcc/support/elapsed.h"
#endif

const char *filename = "monsterdata_test.golden";

#define BENCH_TITLE "monsterdata_test.golden"

#ifdef NDEBUG
#define COMPILE_TYPE "(optimized)"
#else
#define COMPILE_TYPE "(debug)"
#endif

#define FILE_SIZE_MAX (1024 * 10)

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

#define test_assert(x) do { if (!(x)) { assert(0); return -1; }} while(0)

/* A helper to simplify creating buffers vectors from C-arrays. */
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

int verify_parse(void *buffer)
{
    ns(Test_struct_t) test;
    ns(Vec3_struct_t) pos;
    ns(Monster_table_t) monster = ns(Monster_as_root_with_identifier)(buffer, ns(Monster_file_identifier));

    pos = ns(Monster_pos(monster));
    test_assert(pos);
    test_assert(ns(Vec3_x(pos) == 1));
    test_assert(ns(Vec3_y(pos) == 2));
    test_assert(ns(Vec3_z(pos) == 3));
    test_assert(ns(Vec3_test1(pos) == 3.0));
    test_assert(ns(Vec3_test2(pos) == ns(Color_Green)));
    test = ns(Vec3_test3(pos));
    test_assert(test);
    test_assert(ns(Test_a(test)) == 5);
    test_assert(ns(Test_b(test)) == 6);

    // TODO: hp and further fields

    return 0;

}
// TODO:
// when running benchmark with the wrong size argument (output size
// instead of input size), the warmup loop iterates indefinitely in the
// first iteration. This suggests there is an end check missing somwhere
// and this needs to be debugged. The input size as of this writing is 701
// bytes, and the output size is 288 bytes.
int test_parse(void)
{
#if FLATCC_BENCHMARK
    double t1, t2;
    int i;
    int rep = 1000000;
    int warmup_rep = 1000000;
#endif

    const char *buf;
    void *flatbuffer = 0;
    size_t in_size, out_size;
    flatcc_json_parser_t ctx;
    flatcc_builder_t builder;
    flatcc_builder_t *B = &builder;
    int ret = -1;
    flatcc_json_parser_flags_t flags = 0;

    flatcc_builder_init(B);

    buf = readfile(filename, FILE_SIZE_MAX, &in_size);
    if (!buf) {
        fprintf(stderr, "%s: could not read input json file\n", filename);
        return -1;
    }

    if (monster_test_parse_json(B, &ctx, buf, in_size, flags)) {
        goto failed;
    }
    fprintf(stderr, "%s: successfully parsed %d lines\n", filename, ctx.line);
    flatbuffer = flatcc_builder_finalize_aligned_buffer(B, &out_size);
    hexdump("parsed monsterdata_test.golden", flatbuffer, out_size, stdout);
    fprintf(stderr, "input size: %lu, output size: %lu\n",
            (unsigned long)in_size, (unsigned long)out_size);
    verify_parse(flatbuffer);

    cdump("golden", flatbuffer, out_size, stdout);

    flatcc_builder_reset(B);
#if FLATCC_BENCHMARK
    fprintf(stderr, "Now warming up\n");
    for (i = 0; i < warmup_rep; ++i) {
        if (monster_test_parse_json(B, &ctx, buf, in_size, flags)) {
            goto failed;
        }
        flatcc_builder_reset(B);
    }

    fprintf(stderr, "Now benchmarking\n");
    t1 = elapsed_realtime();
    for (i = 0; i < rep; ++i) {
        if (monster_test_parse_json(B, &ctx, buf, in_size, flags)) {
            goto failed;
        }
        flatcc_builder_reset(B);
    }
    t2 = elapsed_realtime();

    printf("----\n");
    show_benchmark(BENCH_TITLE " C generated JSON parse " COMPILE_TYPE, t1, t2, in_size, rep, "1M");
#endif
    ret = 0;

done:
    if (flatbuffer) {
        flatcc_builder_aligned_free(flatbuffer);
    }
    if (buf) {
        free((void *)buf);
    }
    flatcc_builder_clear(B);
    return ret;

failed:
    fprintf(stderr, "%s:%d:%d: %s\n",
            filename, (int)ctx.line, (int)(ctx.error_loc - ctx.line_start + 1),
            flatcc_json_parser_error_string(ctx.error));
    goto done;
}

/* We take arguments so test can run without copying sources. */
#define usage \
"wrong number of arguments:\n" \
"usage: <program> [<input-filename>]\n"

int main(int argc, const char *argv[])
{
    fprintf(stderr, "JSON parse test\n");

    if (argc != 1 && argc != 2) {
        fprintf(stderr, usage);
        exit(1);
    }
    if (argc == 2) {
        filename = argv[1];
    }
    return test_parse();
}
