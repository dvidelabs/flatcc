#define BENCH_TITLE "flatcc for C"

#define BENCHMARK_BUFSIZ 1000
#define DECLARE_BENCHMARK(BM)\
    flatcc_builder_t builder, *BM;\
    BM = &builder;\
    flatcc_builder_init(BM);

#define CLEAR_BENCHMARK(BM) flatcc_builder_clear(BM);


#include "flatbench_builder.h"

#define C(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_FooBarContainer, x)
#define FooBar(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_FooBar, x)
#define Bar(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Bar, x)
#define Foo(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Foo, x)
#define Enum(x) FLATBUFFERS_WRAP_NAMESPACE(benchfb_Enum, x)
#define True flatbuffers_true
#define False flatbuffers_false
#define StringLen flatbuffers_string_len

int encode(flatcc_builder_t *B, void *buffer, size_t *size)
{
    int i, veclen = 3;
    void *buffer_ok;

    flatcc_builder_reset(B);

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

    /*
     * This only works with the default emitter and only if the buffer
     * is larger enough. Otherwise use whatever custom operation the
     * emitter provides.
     */
    buffer_ok = flatcc_builder_copy_buffer(B, buffer, *size);
    *size = flatcc_builder_get_buffer_size(B);
    return !buffer_ok;
}

int64_t decode(flatcc_builder_t *B, void *buffer, size_t size, int64_t sum)
{
    unsigned int i;
    C(table_t) foobarcontainer;
    FooBar(vec_t) list;
    FooBar(table_t) foobar;
    Bar(struct_t) bar;
    Foo(struct_t) foo;

    (void)B;

    foobarcontainer = C(as_root(buffer));
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
