# FlatCC FlatBuffers interface for C

The project includes:

- an executable `flatcc` FlatBuffers schema compiler for C and a
  corresponding library `libflatcc.a`. The compiler generates C header
  files or a binary flatbuffers schema.
- a typeless runtime library `libflatccbuilder.a` for building
  flatbuffers from C. Generated builder headers depend on this library.
  It may also be useful for other language interfaces. The library
  maintains a stack state to make it easy to build buffers from a parser
  or similar.
- an optional portability layer for non-C11 compliant compilers. This is
  not mature and needs target specific testing but should be a good
  starting point for using the generated code with most C compilers
  around.

The `flatcc` compiler is implemented as a standalone tool instead of
extending Googles `flatc` compiler in order to have a pure portable C
library implementation of the schema compiler that is designed to fail
graciously on abusive input in long running processes. It is also
believed a C version may help provide schema parsing to other language
interfaces that find interfacing with C easier than C++. The FlatBuffers
team at Googles FPL lab has been very helpful in providing feedback and
answering many questions to help ensure the best possible compatibility.
Notice the name `flatcc` (FlatBuffers C Compiler) vs Googles `flatc`.

The `flatcc` compiler has some extra features such as allowing for
referencing structs defined later in the schema - which makes sense
since it is already possible with other types. The binary schema format
also does not preserve the order of structs. The option is controlled by
a flag in `config.h` The generated source supports both bottom-up and
top-down construction mixed freely.

`flatcc` does not support parsing JSON objects.


## Time / Space / Usability Tradeoff

The priority has been to design an easy to use C builder interface that
is reasonably fast, suitable for both servers and embedded devices, but
with usability over absolute performance - still the small buffer output
rate is measured in millons per second and read access 10-100 millon
buffers per second from a rough estimate.

For 100MB buffers with 1000 monsters, dynamically extended monster
names, monster vector, and inventory vector, the bandwidth reaches about
2.2GB/s and 45ms/buffer on 2.2GHz Haswell Core i7 CPU. This inlcudes
reading back and validating all data. Reading only a few key fields
increases bandwidth to 2.7GB/s and 37ms/op. For 10MB buffers bandwidth
may be higher but eventually smaller buffers will be hit by call
overhead and thus we down to 300MB/s at about 150ns/op encoding small
buffers. These numbers are just a rough guideline - they obviously
depend on hardware, compiler, and data encoded. Measurements are
excluding an ininitial warmup step.

See also benchmark below.

The client C code can avoid almost any kind of allocation to build
buffers as a builder stack provides an extensible arena before
committing objects - for example appending strings or vectors piecemeal.
The stack is mostly bypassed when a complete object can be constructed
directly such as a vector from integer array on little endian platforms.

The reader interface should be pretty fast as is with less room for
improvement performance wise. It is also much simpler than the builder.

Usability has also been prioritized over smallest possible generated
source code and compile time. It shouldn't affect the compiled size
by much.

The compiled binary output should be reasonably small for everything but
the most restrictive microcontrollers. A 33K monster source test file
(in addition to the generated headers and the builder library) results
in a less than 50K optimized binary executable file including overhead
for printf statements and other support logic, or a 30K object file
excluding the builder library.

Read-only binaries are smaller but not necessarily much smaller
considering they do less work: The compatibility test reads a
pre-generated binary `monsterdata_test.golden` monster file and verifies
that all content is as expected. This results in a 13K optimized binary
executable or a 6K object file. The source for this check is 5K
excluding header files. Readers do not need to link with a library.


## Generated Files

For read-only access to Flatbuffers, no support code is required. The
compiler generates a header file per schema file and a necessary
`flatbuffers_common.h` file which allow for custom name prefixes of all
operations if so desired - this may be relevant if changing offset sizes
in the buffer format.

Reader code is reasonably straight forward and the generated code is
more readable than the builder code because the generated functions
headers are not buried in macros. Refer to `monster_test.c` and the
generated files for detailed guidance on use. The monster schema used in
this project is a slight adaptation to the original to test some
additional edge cases.

For building flatbuffers a separate builder header file is generated per
schema. It requires a `flatbuffers_common_builder.h` file also generated
by the compiler and a small runtime library `libflatccbuilder.a`. It is
because of this requirement that the reader and builder generated code
is kept separate. Typical uses can be seen in the `monster_test.c` file.
The builder allows of repeated pushing of content to a vector or a
string while a containing table is being updated which simplifies
parsing of external formats. It is also possible to build nested buffers
in-line - at first this may sound excessive but it is useful when
wrapping a union of buffers in a network interface and it ensures proper
alignment of all buffer levels.

Low level note: the builder generates all vtables at the end of the
buffer instead of ad-hoc in front of each table but otherwise does the
same deduplication of vtables. This makes it possible to cluster vtables
in hot cache or to make sure all vtables are available when partially
transmitting a buffer. This behavior can be disabled by a runtime flag.

Because some use cases may inlclude very constrained embedded devices,
the builder library can be customized with an allocator object and a
buffer emitter object. The separate emitter ensures a buffer can be
constructed without requiring a full buffer to be present in memory at
once, if so desired.

The typeless builder library is documented in `flatcc_builder.h` and
`flatcc_emitter.h` while the generated typed builder api for C is
documented in `doc/builder.md`.


## Status

The project is still young but test cases cover most functionality
and has been run on OS-X 10.11 with clang and Ubuntu 14.04 with gcc.

There might be alignment bugs not caught be testing but from manual
inspection alignment in buffers looks good so far.

Buffer verification has not been implemented but is planned. It is
expected that verification willl also ensure alignment is correct.
Please note that verification does not ensure buffers can be mutated as
it will not detect overlapping memory regions created maliciously.

Big endian platforms have not been tested at all. While care has been
taken to handle endian encoding, there are bound to be some issues. The
approach taken is to make it work on little endian - then it can always
be made to work on big endian later given that output generated by an
little endian platforms presumable will be correct regardless of bugs in
endian encoding.

The portability layer for older compilers is considered important, but it
is not well tested and it will not be prioritized - rather it will be
updated based on user feedback from target specific testing.

`flatcc` is able encode and decode complex flatbuffer structures as is,
and there are no big known issues apart from the above. The structure of
the common header files are subject to change, notably all functions and
macros prefixed with `__`. The general interface is subject to change
based on feedback but it does appear functional as is. The typeless
flatcc_builder library should hopefully not change much - it is mostly
not user facing but other language exports might want to use it.

There are no plans to make frequent updates once the project becomes
stable, but input from the community will always be welcome and included
in releases where relevant, especially with respect to testing on
different target platforms.


## Using flatcc

Refer to `flatcc -h` for details.

The compiler can either generate a single header file or headers for all
included schema and a common file and with or without support for both
reading (default) and writing (-w) flatbuffers. The simplest option is
to use (-a) for all and include the `myschema_builder.h` file.

Make sure `flatcc` under the `include` folder is visible in the C
compilers include path when compiling flatbuffer builders. It is not
needed for readers without `-DFLATCC_PORTABLE` defined.

The `flatcc` (-I) include path will assume all files with same base name
(case insentive) are identical and only include the first. All generated
files use the input basename and land in working directory or the path
set by (-o).

Note that the binary schema output can be with or without namespace
prefixes and the default differs from `flatc` which strips namespaces.
The binary schema can also have a non-standard size field prefixed so
multiple schema can be concatenated in a single file if so desired (see
also the bfbs2json example).

Files can be generated to stout using (--stdout). C headers will be
ordered and concatenated, but otherwise identical to the separate file
output. Each include statement is guarded so this will not lead to
missing include files. When including builder logic.

The generated code, especially with all combined with --stdout, may
appear large, but only the parts actually used will take space up the
the final executable or object file. Modern compilers inline and include
only necessary parts of the statically linked builder library.

The generated C builder headers normally do require the builder library.
But in reality it is possible to use them without for some purposes. The
main reason is to use `mystruct_assign/copy_from/to_pe` for general
endian handling of structs - eventually also with native big endian for
other network protocols.


## Endianness

The generated code supports the `FLATBUFFERS_LITTLEENDIAN` flag defined
by the `flatc` compiler and it can be used to force endianness. Big
Endian will define it as 0. Other endian may lead to unexpected results.
In most cases `FLATBUFFERS_LITTLEENDIAN` will be defined but in some
cases a decision is made in runtime where the flag cannot be defined.
This is likely just as fast, but `#if FLATBUFFERS_LITTLEENDIAN` can lead
to wrong results alone - use `#if defined(FLATBUFFERS_LITTLEENDIAN) &&
FLATBUFFERS_ENDIAN` to be sure the platform is recognized as little
endian. The detection logic will set `FLATBUFFERS_LITTLEENDIAN` if at
all possible and can be improved with the `pendian.h` file included by
the `-DFLATCC_PORTABLE`.

The user can always define `-DFLATBUFFERS_LITTLEENDIAN` as a compile
time option and then this will take precedence.

It is recommended to use `flatbuffers_is_native_pe()` instead of testing
`FLATBUFFERS_LITTLEENDIAN` whenever it can be avoided to use the
proprocessor for several reasons:

- if it isn't defined, the source won't compile at all
- combined with `pendian.h` it provides endian swapping even without
  preprocessor detection.
- it is normally a constant similar to `FLATBUFFERS_LITTLEENDIAN`
- it works even with undefined `FLATBUFFERS_LITTLEENDIAN`
- compiler should optimize out even runtime detection
- protocol might not always be little endian.
- it is defined as a macro and can be checked for existence.

`pe` means protocol endian. This suggests that `flatcc` output may be
used for other protocols in the future, or for variations of
flatbuffers. The main reason for this is the generated structs that can
be very useful on other predefined network protols in big endian. Each
struct has a `mystruct_copy_from_pe` method and similar to do these
conversions. Internally flatcc optimizes struct conversions by testing
`flatbuffers_is_native_pe()` in some heavier struct conversions.

In a few cases it may be relevant to test for `FLATBUFFERS_LITTLEENDIAN`
but then code intended for general use should provide an alternitive for
when the flag isn't defined.

Even with correct endian detection, the code may break on platforms
where `flatbuffers_is_native_pe()` is false because the necessary system
headers could not found. In this case `-DFLATCC_PORTABLE` should help.


## Offset Sizes and Direction

FlatBuffers use 32-bit `uoffset_t` and 16-bit `voffset_t`. `soffset_t`
always has the same size as `uoffset_t`. These types can be changed by
preprocessor defines without regenerating code. However, it is important
that `libflatccbuilder.a` is compiled with the same types as defined in
`flatcc_types.h`.

`uoffset_t` currently always point forward like `flatc`. In retrospect
it would probably have simplified buffer constrution if offsets pointed
the opposite direction or were allowed to be signed. This is a major
change and not likely to happen for reasons of effort and compatibility,
but it is worth keeping in mind for a v2.0 of the format.

Vector header fields storing the lengthare defined as `uoffset_t` which
is 32-bit wide by default. If `uoffset_t` is redefined this will
therefore also affect vectors and strings.

The practical buffer size is limited to about half of the `uoffset_t` range
because vtable references are signed which in effect means that buffers
are limited to about 2GB by default.


## Quickstart - reading a buffer

Here we provide a quick example of read-only access to Monster flatbuffer
- it is an adapted extract of the `monster_test.c` file.

First we compile the schema read-only with common (-c) support header and we
add the recursion because `monster_test.fbs` includes other files.

    flatcc -cr monster_test.fbs

we get:

    flatbuffers_common.h
    include_test1.h
    include_test2.h
    monster_test.h

Namespaces can be long so we use a macro to manage this.


    #include "monster_test.h"

    #undef ns
    #define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

    int verify_monster(void *buffer)
    {
        ns(Monster_table_t) monster;
        /* This is a read-only reference to a flatbuffer encoded struct. */
        ns(Vec3_struct_t) vec;
        flatbuffers_string_t name;
        size_t offset;

        if (!(monster = ns(Monster_as_root(buffer)))) {
            printf("Monster not available\n");
            return -1;
        }
        if (ns(Monster_hp(monster)) != 80) {
            printf("Health points are not as expected\n");
            return -1;
        }
        if (!(vec = ns(Monster_pos(monster)))) {
            printf("Position is absent\n");
            return -1;
        }

        /* -3.2f is actually -3.20000005 and not -3.2 due to tation loss. */
        if (ns(Vec3_z(vec)) != -3.2f) {
            printf("Position failing on z coordinate\n");
            return -1;
        }

        /* Verify force_align relative to buffer start. */
        offset = (char *)vec - (char *)buffer;
        if (offset & 15) {
            printf("Force align of Vec3 struct not correct\n");
            return -1;
        }

        /*
         * If we retrieved the buffer using `flatcc_builder_finalize_aligned_buffer` or
         * `flatcc_builder_get_direct_buffer` the struct should also
         * be aligned without subtracting the buffer.
         */
        if (vec & 15) {
            printf("warning: buffer not aligned in memory\n");
        }

        /* ... */
        return 0;
    }
    /* main() {...} */

Assuming our above file is `monster_test.c` the following are a few ways to compile the project:

    cc monster_example.c -o monster_example

    cc --std=c11 monster_test.c -o monster_test

    cc -D FLATCC_PORTABLE -I include monster_test.c -o monster_test

The portability layer requires the include directive to find the support files - these
are all header files.


## Quickstart - wrapping type names

We can also create a type specific namespace wrapper - which also works
with the builder below:


    #define Monster(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example_Monster, x)
    ...
    assert(Monster(hp(monster)) == 80);

or for even easer read access if we assume monster is stored in `monster`.

    #define MonsterGet(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example_Monster, x(monster))
    ...
    assert(MonsterGet(hp) == 80);

all without loosing type safety.


## Quickstart - building a buffer

Here we provide a very limited example of how to build a buffer - only a few
fields are updated. Pleaser refer to `monster_test.c` and the doc directory
for more information.

First we must generate the files:

    flatcc -a monster_test.fbs

This produces:

    flatbuffers_common.h
    flatbuffers_common_builder.h
    include_test1.h
    include_test1_builder.h
    include_test2.h
    include_test2_builder.h
    monster_test.h
    monster_test_builder.h

Note: we wouldn't actually do the readonly generation shown earlier unless we only intend to
read buffers - the builder generation always generates read acces too.

By including `"monster_test_builder.h"` all other files are included
automatically. The C compiler needs the -I include directive to access
`flatbuffers/flatcc_builder.h` and related.

The builder must be initialized first to set up the runtime environment we need
for building buffers efficiently - the builder depends on an emitter object to
construct the actual buffer - here we implicitly use the default. Once we have
that, we can just consider the builder a handle and focus on the FlatBuffers
generated API until we finalize the buffer (i.e. access the result).
For non-trivial uses it is recommended to provide a custom emitter and
for example emit pages over the network as soon as they complete rather than
merging all pages into a single buffer using `flatcc_builder_finalize_buffer`,
or the simplistic direct buffer access shown below. See also
`flatcc_builder.h` and `flatcc_emitter.h`.


    #include "monster_test_builder.h"

    /* See `monster_test.c` for more advanced examples. */
    void build_monster(flatcc_builder_t *B)
    {
        ns(Vec3_t *vec);

        /* Here we use a table, but structs can also be roots. */
        ns(Monster_start_as_root(B));

        ns(Monster_hp_add(B, 80));
        /* The vec struct is zero-initalized. */
        vec = ns(Monster_pos_start(B));
        /* Native endian. */
        vec->x = 1, vec->y = 2, vec->z = -3.2f;
        /* _end call converts to protocol endian format - for LE it is a nop. */
        ns(Monster_pos_end(B));
        /* Name is required, or we get an assertion in debug builds. */
        ns(Monster_name_create_str(B, "MyMonster"));
        ns(Monster_pos_end(B));

        ns(Monster_end_as_root(B));
    }

    #include "test/support/hexdump.h"

    int main(int argc, char *argv[])
    {
        flatcc_builder_t builder;
        void *buffer;
        size_t size;

        flatcc_builder_init(&builder);

        build_monster(&builder);
        /* We could also use `flatcc_builder_finalize_buffer` and free the buffer later. */
        buffer = flatcc_builder_get_direct_buffer(&builder, &size);
        assert(buffer);
        verify_monster(buffer);

        /* Visualize what we got ... */
        hexdump("monster example", buffer, size, stdout);

        /*
         * Here we can call `flatcc_builder_reset(&builder) if
         * we wish to build more buffers before deallocating
         * internal memory with `flatcc_builder_clear`.
         */

        flatcc_builder_clear(&builder);
        return 0;
    }

Compile the example project:

    cc --std=c11 -I include monster_example.c lib/libflatccbuilder.a -o monster_example

Note that here the include directive is required even without the portability
layer so that `flatbuffers/flatcc_builder.h` can be found.

## Required Fields and Duplicate Fields

If a field is required such as Monster.name, the table end call will
assert in debug mode and create incorrect tables in non-debug builds.
The assertion may not be easy to decipher as it happens in library code
and it will not tell which field is missing.

When reading the name, debug mode will again assert and non-debug builds
will return a default value.

Writing the same field twice will also trigger an assertion in debug
builds.

## Fast Buffers

Buffers can be used for high speed communication by using the ability to
create buffers with structs as root. In addition the default emitter
supports `flatcc_emitter_direct_buffer` for small buffers so no extra copy
step is required to get a linear buffer in memory. Preliminary
measurements suggests there is a limit to how fast this can go (about
6-7 mill. buffers/sec) because the builder object must be reset between
buffers which involves zeroing allocated buffers. Small tables with a
simple vector achieve roughly half that speed. For really high speed a
dedicated builder for structs would be needed. See also
`monster_test.c`.


## Types

All types stored in a buffer has a type suffix such as `Monster_table_t`
or `Vec3_struct_t` (and namespace prefix which we leave out here). These
types are read-only pointers into endian encoded data. Enum types are
just constants easily grasped from the generated code. Tables are dense so
they are never accessed directly.

Structs have a dual purpose because they are also valid types in native
format, yet the native reprsention has a slightly different purpose.
Thus the convention is that a const pointer to a struct encoded in a
flatbuffer has the type `Vec3_struct_t` where as a writeable pointer to
a native struct has the type `Vec3_t *` or `struct Vec3 *`.

Union types are just any of a set of possible table types and an enum
named as for example `Any_union_type_t`. There is a compound union type
that can store both type and table reference such that `create` calls
can represent unions as a single argument - see `flatcc_builder.h` and
`doc/builder.md`. Union table fields return a pointer of type
`flatbuffers_generic_table_t` which is defined as `const void *`.

All types have a `_vec_t` suffix which is a const pointer to the
underlying type. For example `Monster_table_t` has the vector type
`Monster_vec_t`. There is also a non-const variant with suffix
`_mutable_vec_t` which is rarely used. However, it is possible to sort
vectors in-place in a buffer, and for this to work, the vector must be
cast to mutable first. A vector (or string) type points to the element
with index 0 in the buffer, just after the length field, and it may be
cast to a native type for direct access with attention to endian
encoding. (Note that `table_t` types do point to the header field unlike
vectors.) These types are all for the reader interface. Corresponding
types with a `_ref_t` suffix such as `_vec_ref_t` are used during
the construction of buffers.

Native scalar types are mapped from the FlatBuffers schema type names
such as ubyte to `uint8_t` and so forth. These types also have vector
types provided in the common namespace (default `flatbuffers_`) so
a `[ubyte]` vector has type `flatbuffers_uint8_vec_t` which is defined
as `const uint8_t *`.

The FlatBuffers boolean type is strictly 8 bits wide so we cannot use or
emulate `<stdbool.h>` where `sizeof(bool)` is implementation dependent.
Therefore `flatbuffers_bool_t` is defined as `uint8_t` and used to
represent FlatBuffers boolean values and the constants of same type:
`flatbuffers_true = 1` and `flatbuffers_false = 0`. Even so,
`pstdbool.h` is available in the `include/portable` directory if `bool`,
`true`, and `false` are desired in user code and `<stdbool.h>` is
unavailable.

`flatbuffers_string_t` is `const char *` but imply the returned pointer
has a length prefix just before the pointer. `flatbuffers_string_vec_t`
is a vector of strings. The `flatbufers_string_t` type guarantees that a
length field is present using `flatbuffers_string_len(s)` and that the
string is zero terminated. It also suggests that it is in utf-8 format
according to the FlatBuffers specification, but not checks are done and
the `flatbuffers_create_string(B, s, n)` call explicitly allows for
storing embedded null characters and other binary data.

All vector types have operations defined as the typename with `_vec_t`
replaced by `_vec_at` and `_vec_len`. For example
`flatbuffers_uint8_vec_at(inv, 1)` or `Monster_vec_len(inv)`. The length
or `_vec_len` will be 0 if the vector is missing whereas `_vec_at` will
assert in debug or behave undefined in release builds following out of
bounds access. This also applies to related string operations.

To be consistent with a user define namespace ns, the common namespace
can also be wrapped in a macro in exactly the same way:

    #undef nsc
    #define nsc(x) FLATBUFFERS_WRAP_NAMESPACE(flatbuffers, x)

    nsc(uint8_vec_t) vec;

The common namespace can also be redefined using the `flatcc
--common-prefx myprefix` argument to change `flatbuffers_` to something
else. In that case the macro above becomes:


    #undef nsc
    #define nsc MYPREFIX_WRAPNAMESPACE(myprefix, x)

One reason to change the common namespace is if the flatbuffer
offset types are changed, for example to handle large 64-bit binary
chunks. Note however that the builder library can currently only be
compiled with one specific set of types.


## Pitfalls in Error Handling

The builder API often returns a reference or a pointer where null is
considered an error or at least a missing object default. However, some
operations do not have a meaningful object or value to return. These
follow the convention of 0 for success and non-zero for failure.
Also, if anything fails, it is not safe to proceed with building a
buffer.  However, to avoid overheads, there is no hand holding here. On
the upside, failures only happen with incorrect use or allocation
failure and since the allocator can be customized, it is possible to
provide a central error state there or to guarantee no failure will
happen depending on use case, assuming the API is otherwise used
correctly.  By not checking error codes, this logic also optimizes out
for better performance.


## Sorting and Finding

The builder API does not support sorting due to the complexity of
customizable emitters, but the reader API does support sorting so a
buffer can be sorted at a later stage. This requires casting a vector to
mutable and calling the sort method available for fields with keys.

The sort uses heap sort and can sort a vector in-place without using
external memory or recursion. Due to the lack of external memory, the
sort is not stable. The corresponding find operation returns the lowest
index of any matching key, or `flatbuffers_not_found`.

When configured in `config.h`, the `flatcc` compiler allows multiple
keyed fields unlike Googles `flatc` compiler. This works transparently
by providing `<table_name>_vec_sort_by_<field_name>` and
`<table_name>_vec_find_by_<field_name>` methods for all keyed fields. The
first field maps to `<table_name>_vec_sort` and `<table_name>_vec_find`.
Obviously the chosen find method must match the chosen sort method.

See also `doc/builder.md` and `test/monster_test/monster_test.c`.


## Null Values

The FlatBuffers format does not fully distinguish between default values
and missing or null values but it is possible to force values to be
written to the buffer.  This is discussed further in the `builder.md`.
For SQL data roundtrips this may be more important that having compact
data.

The `_is_present` suffix on table access methods can be used to detect if
value is present in a vtable, for example `Monster_hp_present`. Unions
return true of the type field is present, even if it holds the value
None.

The `add` methods have corresponding `force_add` methods for scalar and enum
values to force storing the value even if it is default and thus making
it detectable by `is_present`.


## Portability Layer

The portablity layer is not required when --std=c11 is defined on a clang
compiler where little endian is avaiable and easily detected, or where
`<endian.h>` is available and easily detected. `flatcc_builder_common.h`
contains a minimal portability abstraction that also works for some
platforms even without C11, e.g. OS-X clang. The portablity file
can be included before other headers, or by setting the compile time
directives:

    cc -D FLATCC_PORTABLE -I include monster_test.c -o monster_test

Also see the top of this file on how to include the actual portability layer
when needed.

If a specific platform has been tested, it would be good with feedback
and possibly patches to the portability layer so this can be documented
for other users.

## Building

To initialize and run the build:


    scripts/build.sh

The `bin` and `lib` folders will be created with debug and release
builds.

The build depends on CMake and the Ninja build tool to be
installed along with Python. The Ninja build backend can be replaced by
editing `scripts/initbuild.sh` and `scripts/build.sh`.

To install build tools on OS-X:

    brew update
    brew install cmake ninja

To install build tools on Ubuntu:

    sudo apt-get update
    sudo apt-get install cmake ninja-build


## Testing

Run

    scripts/test.sh

It will also initialize the build if needed. Several sub-tests have
shell scripts that can be run in isolation. A large dataset test is part
of the test, but a separate benchmark is not. See benchmark section.

The script must end with `TEST PASSED`, or it didn't.


## Limited Windows Support

Windows hasn't been tested but the portability layer does include
abstractions to support the Windows platform, such as defining inline as
`__inline` and providing `<endian.h>` abstractions. However, the Windows
Visual Studio platform is fairly focused on C++ so it wouldn't be the
primary target.

With clang being ported to Windows there shouldn't be much reason for
any portablitiy issues using that tool chain - although clangs runtime
libraries do depend on the platform affecting the degree to which C11 is
supported.

The build scripts are shell scripts wrapping CMake, but CMake can also
be used directly. The test scripts are more involved and expect a bash
shell, but this is also available on Windows.

The file logic in the compiler may fail on Windows paths as this hasn't
been tested, but at least it has been implemented with Windows in mind.


## Using the Compiler and Builder library

The compiler library `libflatcc.a` can compile schemas provided
in a memory buffer or as a filename. When given as a buffer, the schema
cannot contain include statements - these will cause a compile error.

When given a filename the behavior is similar to the commandline
`flatcc` interface, but with more options - see `flatcc.h` and
`config/config.h`.

The binary schema options are named `bgen_...` and can be used to
choose namespace prefixes or not.

`libflatcc.a` supports functoins named `flatcc_...`. `reflection...` may
also be available which are simple the C generated interface for the
binary schema. The builder library is also included. These last two
interfaces are only present because the library supports binary schema
generation.

The standalone `libflatcc_builder.a` is much smaller and may in fact be
linked directly as a single `flatcc_builder.c` file, optionally also with
`flatcc_emitter.c`. The builder libraries primary funciton is to support
the generated C builder wrappers, but it can do more things: for example
creating vtables ahead of time and creating tables with a specific
vtable. This may be used to create high performance applications. The
library is also typeless as the wrappers provide actual types. Therefore
dynamic languages can use it to construct flatbuffers on the fly, and
the same applies to generic JSON, XML or similar parsers. Of course,
some schema is eventually required to understand the generated output
but even that might be generated on the fly by a parser or language
interface.


## Benchmark

Benchmarks are defined for raw C structs, Googles `flatc` generated C++
and the `flatcc` compilers C ouput.

These can be run with:

    scripts/benchmark.sh

and requires a C++ compiler installed - the benchmark for flatcc alone can be
run with:

    test/benchmark/benchflatcc/run.sh

this only requires a system C compiler (cc) to be installed (and
flatcc's build environment).

A summary for OS-X 2.2 GHz Haswell core i7 is found below. Generated
files for OS-X and Ubuntu are found in the benchmark folder.

The benchmarks use the same schema and dataset as Google FPL's
FlatBuffers benchmark.

In summary, 1 million iterations runs at about 500-540MB/s at 620-700
ns/op encoding buffers and 29-34ns/op traversing buffers. `flatc` and
`flatcc` are close enough in performance for it not to matter much.
`flatcc` is a bit faster encoding but it is likely due to less memory
allocation. Throughput and time per operatin are of course very specific
to this test case.


### operation: flatbench for raw C structs encode (optimized)
    elapsed time: 0.055 (s)
    iterations: 1000000
    size: 312 (bytes)
    bandwidth: 5665.517 (MB/s)
    throughput in ops per sec: 18158707.100
    throughput in 1M ops per sec: 18.159
    time per op: 55.070 (ns)

### operation: flatbench for raw C structs decode/traverse (optimized)
    elapsed time: 0.012 (s)
    iterations: 1000000
    size: 312 (bytes)
    bandwidth: 25978.351 (MB/s)
    throughput in ops per sec: 83263946.711
    throughput in 1M ops per sec: 83.264
    time per op: 12.010 (ns)

### operation: flatc for C++ encode (optimized)
    elapsed time: 0.702 (s)
    iterations: 1000000
    size: 344 (bytes)
    bandwidth: 490.304 (MB/s)
    throughput in ops per sec: 1425301.380
    throughput in 1M ops per sec: 1.425
    time per op: 701.606 (ns)

### operation: flatc for C++ decode/traverse (optimized)
    elapsed time: 0.029 (s)
    iterations: 1000000
    size: 344 (bytes)
    bandwidth: 11917.134 (MB/s)
    throughput in ops per sec: 34642832.398
    throughput in 1M ops per sec: 34.643
    time per op: 28.866 (ns)


### operation: flatcc for C encode (optimized)
    elapsed time: 0.626 (s)
    iterations: 1000000
    size: 336 (bytes)
    bandwidth: 536.678 (MB/s)
    throughput in ops per sec: 1597255.277
    throughput in 1M ops per sec: 1.597
    time per op: 626.074 (ns)

### operation: flatcc for C decode/traverse (optimized)
    elapsed time: 0.029 (s)
    iterations: 1000000
    size: 336 (bytes)
    bandwidth: 11726.930 (MB/s)
    throughput in ops per sec: 34901577.551
    throughput in 1M ops per sec: 34.902
    time per op: 28.652 (ns)

