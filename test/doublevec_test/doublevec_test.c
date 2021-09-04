#include <stdio.h>

#include "doublevec_test_builder.h"
#include "doublevec_test_verifier.h"

#include "flatcc/support/hexdump.h"
#include "flatcc/support/elapsed.h"
#include "../../config/config.h"

#undef nsc
#define nsc(x) FLATBUFFERS_WRAP_NAMESPACE(flatbuffers, x)

int gen_doublevec(flatcc_builder_t *B)
{
    flatcc_builder_reset(B);

    DoubleVec_start_as_root(B);
    DoubleVec_a_create(B, 0, 0);
    DoubleVec_end_as_root(B);

    return 0;
}

int gen_doublevec_with_size(flatcc_builder_t *B)
{
    flatcc_builder_reset(B);

    DoubleVec_start_as_root_with_size(B);
    DoubleVec_a_create(B, 0, 0);
    DoubleVec_end_as_root(B);

    return 0;
}

int test_doublevec(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    int ret;

    gen_doublevec(B);

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("doublevec table", buffer, size, stderr);

    if ((ret = DoubleVec_verify_as_root(buffer, size))) {
        printf("doublevec buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }

    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_doublevec_with_size(flatcc_builder_t *B)
{
    void *buffer, *frame;
    size_t size, size2, esize;
    int ret;

    gen_doublevec_with_size(B);

    frame = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("doublevec table with size", frame, size, stderr);

    buffer = flatbuffers_read_size_prefix(frame, &size2);
    esize = size - sizeof(flatbuffers_uoffset_t);
    if (size2 != esize) {
        printf("Size prefix has unexpected size, got %i, expected %i\n", (int)size2, (int)esize);
        return -1;
    }

    if ((ret = DoubleVec_verify_as_root(buffer, size))) {
        printf("doublevec buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }

    flatcc_builder_aligned_free(frame);
    return ret;
}


int main(int argc, char *argv[])
{
    flatcc_builder_t builder, *B;

    (void)argc;
    (void)argv;

    B = &builder;
    flatcc_builder_init(B);

#ifdef NDEBUG
    printf("running optimized doublevec test\n");
#else
    printf("running debug doublevec test\n");
#endif
#if 1
    if (test_doublevec(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_doublevec_with_size(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif

    flatcc_builder_clear(B);
    return 0;
}
