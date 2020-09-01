#include <assert.h>
#include <stdio.h>

#include "optional_scalars_test_builder.h"


#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(optional_scalars, x)

// #define TEST_ASSERT

#ifdef TEST_ASSERT
#define test_assert(x) do { if (!(x)) { assert(0); return -1; }} while(0)
#else
#define test_assert(x) do { if (!(x)) { return -1; }} while(0)
#endif

int create_scalar_stuff(flatcc_builder_t *B)
{
    ns(ScalarStuff_start_as_root(B));

    ns(ScalarStuff_just_i8_add(B, 10));
    ns(ScalarStuff_maybe_i8_add(B, 11));
    ns(ScalarStuff_default_i8_add(B, 12));

    ns(ScalarStuff_end_as_root(B));

    return 0;
}

int access_scalar_stuff(const void *buffer)
{
    ns(ScalarStuff_table_t) t = ns(ScalarStuff_as_root(buffer));
    flatbuffers_int8_option_t maybe_i8;
    ns(OptionalByte_option_t) maybe_enum;

    test_assert(10 == ns(ScalarStuff_just_i8_get(t)));
    test_assert(11 == ns(ScalarStuff_maybe_i8_get(t)));
    test_assert(12 == ns(ScalarStuff_default_i8_get(t)));

    maybe_i8 = ns(ScalarStuff_maybe_i8_option(t));
    test_assert(!maybe_i8.is_null);
    test_assert(11 == maybe_i8.value);
    test_assert(ns(ScalarStuff_just_i8_is_present(t)));
    test_assert(ns(ScalarStuff_maybe_i8_is_present(t)));
    test_assert(ns(ScalarStuff_default_i8_is_present(t)));

    maybe_enum = ns(ScalarStuff_maybe_enum_option(t));
    test_assert(maybe_enum.is_null);

    return 0;
}

int test()
{
    flatcc_builder_t builder;
    void  *buf;
    size_t size;

    flatcc_builder_init(&builder);
    test_assert(0 == create_scalar_stuff(&builder));
    buf = flatcc_builder_finalize_aligned_buffer(&builder, &size);
    test_assert(0 == access_scalar_stuff(buf));
    flatcc_builder_aligned_free(buf);
    flatcc_builder_clear(&builder);

    return 0;
}

int main(int argc, char *argv[])
{
    /* Silence warnings. */
    (void)argc;
    (void)argv;

    if (test()) {
        printf("optional scalars test failed");
        return 1;
    } else {
        printf("optional scalars test passed");
        return 0;
    }
}

