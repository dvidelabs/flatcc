#include <stdio.h>

/* Only needed for verification. */
#include "monster_test_json_printer.h"
#include "support/readfile.h"
#include "support/elapsed.h"

#define FLATCC_BENCHMARK 0

#define BENCH_TITLE "monsterdata_test.mon/json-printer"

#ifdef NDEBUG
#define COMPILE_TYPE "(optimized)"
#else
#define COMPILE_TYPE "(debug)"
#endif

#define FILE_SIZE_MAX (1024 * 10)

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

/* A helper to simplify creating buffers vectors from C-arrays. */
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

int test_print()
{
    int ret = 0;
    const char *buf = 0;
    const char *golden = 0;
    const char *target = 0;
    const char *filename = "monsterdata_test.mon";
    const char *golden_filename = "monsterdata_test.golden";
    const char *target_filename = "monsterdata_test.json.txt";
    size_t size = 0, golden_size = 0, target_size = 0;
    flatcc_json_printer_t ctx_obj, *ctx;
    FILE *fp = 0;

    ctx = &ctx_obj;

    fp = fopen(target_filename, "wb");
    if (!fp) {
        fprintf(stderr, "%s: could not open output file\n", target_filename);
        /* ctx not ready for clenaup, so exit directly. */
        return -1;
    }
    flatcc_json_printer_init(ctx, fp);
    /* Uses same formatting as golden reference file. */
    flatcc_json_printer_set_nonstrict(ctx);

    buf = read_file(filename, FILE_SIZE_MAX, &size);
    if (!buf) {
        fprintf(stderr, "%s: could not read input flatbuffer file\n", filename);
        goto fail;
    }
    golden = read_file(golden_filename, FILE_SIZE_MAX, &golden_size);
    if (!golden) {
        fprintf(stderr, "%s: could not read verification json file\n", golden_filename);
        goto fail;
    }
    ns(Monster_print_json_as_root(ctx, buf, size, "MONS"));
    flatcc_json_printer_flush(ctx);
    if (flatcc_json_printer_get_error(ctx)) {
        printf("could not print monster data\n");
    }
    fclose(fp);
    fp = 0;
    target = read_file(target_filename, FILE_SIZE_MAX, &target_size);
    if (!target) {
        fprintf(stderr, "%s: could not read back output file\n", target_filename);
        goto fail;
    }
    if (target_size != golden_size || memcmp(target, golden, target_size)) {
        fprintf(stderr, "generated output file did not match verification file\n");
        goto fail;
    }
    fprintf(stderr, "json print test succeeded\n");

done:
    flatcc_json_printer_clear(ctx);
    if (buf) {
        free((void *)buf);
    }
    if (golden) {
        free((void *)golden);
    }
    if (target) {
        free((void *)target);
    }
    if (fp) {
        fclose(fp);
    }
    return ret;
fail:
    ret = -1;
    goto done;
}

int main(int argc, const char *argv[])
{
    (void)argc;
    (void)argv;

    fprintf(stderr, "running json print test\n");
    return test_print();
}

