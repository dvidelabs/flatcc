#define BENCH_TITLE "flatc for C++"

#define BENCHMARK_BUFSIZ 1000
#define DECLARE_BENCHMARK(BM)\
    void *BM = 0
#define CLEAR_BENCHMARK(BM)

#include <string.h>
#include "flatbench_generated.h"

using namespace flatbuffers;
using namespace benchfb;

/* The builder is created each time - perhaps fbb can be reused somehow? */
int encode(void *bench, void *buffer, size_t *size)
{
    const int veclen = 3;
    Offset<FooBar> vec[veclen];
    FlatBufferBuilder fbb;

    (void)bench;

    for (int i = 0; i < veclen; i++) {
        // We add + i to not make these identical copies for a more realistic
        // compression test.
        auto const &foo = Foo(0xABADCAFEABADCAFE + i, 10000 + i, '@' + i, 1000000 + i);
        auto const &bar = Bar(foo, 123456 + i, 3.14159f + i, 10000 + i);
        auto name = fbb.CreateString("Hello, World!");
        auto foobar = CreateFooBar(fbb, &bar, name, 3.1415432432445543543 + i, '!' + i);
        vec[i] = foobar;
    }
    auto location = fbb.CreateString("https://www.example.com/myurl/");
    auto foobarvec = fbb.CreateVector(vec, veclen);
    auto foobarcontainer = CreateFooBarContainer(fbb, foobarvec, true, Enum_Bananas, location);
    fbb.Finish(foobarcontainer);
    if (*size < fbb.GetSize()) {
        return -1;
    }
    *size = fbb.GetSize();
    memcpy(buffer, fbb.GetBufferPointer(), *size);
    return 0;
}

int64_t decode(void *bench, void *buffer, size_t size, int64_t sum)
{
    auto foobarcontainer = GetFooBarContainer(buffer);

    (void)bench;
    sum += foobarcontainer->initialized();
    sum += foobarcontainer->location()->Length();
    sum += foobarcontainer->fruit();
    for (unsigned int i = 0; i < foobarcontainer->list()->Length(); i++) {
        auto foobar = foobarcontainer->list()->Get(i);
        sum += foobar->name()->Length();
        sum += foobar->postfix();
        sum += static_cast<int64_t>(foobar->rating());
        auto bar = foobar->sibling();
        sum += static_cast<int64_t>(bar->ratio());
        sum += bar->size();
        sum += bar->time();
        auto &foo = bar->parent();
        sum += foo.count();
        sum += foo.id();
        sum += foo.length();
        sum += foo.prefix();
    }
    return sum + 2 * sum;
}

#include "benchmain.h"
