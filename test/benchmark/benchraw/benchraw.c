#define BENCH_TITLE "flatbench for raw C structs"

#define BENCHMARK_BUFSIZ 1000
#define DECLARE_BENCHMARK(BM)\
    void *BM = 0
#define CLEAR_BENCHMARK(BM) 

#include <string.h>
#include <stdint.h>

#define STRING_LEN 32
#define VEC_LEN 3
#define fb_bool uint8_t

enum Enum { Apples, Pears, Bananas };

struct Foo {
  int64_t id;
  short count;
  char prefix;
  int length;
};

struct Bar {
  struct Foo parent;
  int time;
  float ratio;
  unsigned short size;
};

struct FooBar {
  struct Bar sibling;
  int name_len;
  char name[STRING_LEN];
  double rating;
  unsigned char postfix;
};

struct FooBarContainer {
  struct FooBar list[VEC_LEN];
  fb_bool initialized;
  enum Enum fruit;
  int location_len;
  char location[STRING_LEN];
};

int encode(void *bench, void *buffer, size_t *size)
{
    int i;
    struct FooBarContainer fbc;
    struct FooBar *foobar;
    struct Foo *foo;
    struct Bar *bar;

    (void)bench;

    strcpy(fbc.location, "https://www.example.com/myurl/");
    fbc.location_len = strlen(fbc.location);
    fbc.fruit = Bananas;
    fbc.initialized = 1;
    for (i = 0; i < VEC_LEN; ++i) {
      foobar = &fbc.list[i];
      foobar->rating = 3.1415432432445543543 + i;
      foobar->postfix = '!' + i;
      strcpy(foobar->name, "Hello, World!");
      foobar->name_len = strlen(foobar->name);
      bar = &foobar->sibling;
      bar->ratio = 3.14159f + i;
      bar->size = 10000 + i;
      bar->time = 123456 + i;
      foo = &bar->parent;
      foo->id = 0xABADCAFEABADCAFE + i;
      foo->count = 10000 + i;
      foo->length = 1000000 + i;
      foo->prefix = '@' + i;
    }
    if (*size < sizeof(struct FooBarContainer)) {
        return -1;
    }
    *size = sizeof(struct FooBarContainer);
    memcpy(buffer, &fbc, *size);
    return 0;
}

int64_t decode(void *bench, void *buffer, size_t size, int64_t sum)
{
    int i;
    struct FooBarContainer *foobarcontainer;
    struct FooBar *foobar;
    struct Foo *foo;
    struct Bar *bar;

    (void)bench;

    foobarcontainer = buffer;
    sum += foobarcontainer->initialized;
    sum += foobarcontainer->location_len;
    sum += foobarcontainer->fruit;
    for (i = 0; i < VEC_LEN; ++i) {
        foobar = &foobarcontainer->list[i];
        sum += foobar->name_len;
        sum += foobar->postfix;
        sum += (int64_t)foobar->rating;
        bar = &foobar->sibling;
        sum += (int64_t)bar->ratio;
        sum += bar->size;
        sum += bar->time;
        foo = &bar->parent;
        sum += foo->count;
        sum += foo->id;
        sum += foo->length;
        sum += foo->prefix;
    }
    return sum + 2 * sum;
}

#include "benchmain.h"
