#include <stdlib.h>
#include <assert.h>

#include "monster_test_reader.h"
#include "monster_test_verifier.h"
#include "flatcc/support/readfile.h"
#include "flatcc/support/hexdump.h"

#define align_up(alignment, size)                                           \
    (((size_t)(size) + (size_t)(alignment) - 1) & ~((size_t)(alignment) - 1))

const char *filename = "monsterdata_test.mon";

#undef ns
#define ns(x) MyGame_Example_ ## x

int verify_monster(void *buffer)
{
    ns(Monster_table_t) monster, mini;
    ns(Vec3_struct_t) vec;
    ns(Test_struct_t) test;
    ns(Test_vec_t) testvec;
    ns(Any_union_type_t) mini_type;
    flatbuffers_string_t name;
    size_t offset;
    flatbuffers_uint8_vec_t inv;
    flatbuffers_string_vec_t aofs;
    flatbuffers_string_t s;
    size_t i;

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
    offset = (size_t)((char *)vec - (char *)buffer);
    if (offset & 15) {
        printf("Force align of Vec3 struct not correct\n");
        return -1;
    }
    if (ns(Vec3_x(vec)) != 1) {
        printf("Position failing on x coordinate\n");
        return -1;
    }
    if (ns(Vec3_y(vec)) != 2) {
        printf("Position failing on y coordinate\n");
        return -1;
    }
    if (ns(Vec3_z(vec)) != 3) {
        printf("Position failing on z coordinate\n");
        return -1;
    }
    if (ns(Vec3_test1(vec)) != 3) {
        printf("Vec3 test1 is not 3\n");
        return -1;
    }
    if (ns(Vec3_test2(vec)) != ns(Color_Green)) {
        printf("Vec3 test2 not Green\n");
        return -1;
    }
    test = ns(Vec3_test3(vec));
    if (ns(Test_a(test)) != 5 || ns(Test_b(test) != 6)) {
        printf("test3 not valid in Vec3\n");
        return -1;
    }
    name = ns(Monster_name(monster));
    if (flatbuffers_string_len(name) != 9) {
        printf("Name length is not correct\n");
        return -1;
    }
    if (!name || strcmp(name, "MyMonster")) {
        printf("Name is not correct\n");
        return -1;
    }
    inv = ns(Monster_inventory(monster));
    if (flatbuffers_uint8_vec_len(inv) != 5) {
        printf("Inventory has wrong length\n");
        return -1;
    }
    for (i = 0; i < 5; ++i) {
        if (flatbuffers_uint8_vec_at(inv, i) != i) {
            printf("Inventory item #%d is wrong\n", (int)i);
            return -1;
        }
    }
    if (!(aofs = ns(Monster_testarrayofstring(monster)))) {
        printf("Array of string not present\n");
        return -1;
    }
    if (flatbuffers_string_vec_len(aofs) != 2) {
        printf("Array of string has wrong vector length\n");
        return -1;
    }
    s = flatbuffers_string_vec_at(aofs, 0);
    if (strcmp(s, "test1")) {
        printf("First string array element is wrong\n");
        return -1;
    }
    s = flatbuffers_string_vec_at(aofs, 1);
    if (strcmp(s, "test2")) {
        printf("Second string array element is wrong\n");
        return -1;
    }
    mini_type = ns(Monster_test_type(monster));
    if (mini_type != ns(Any_Monster)) {
        printf("Not any monster\n");
        return -1;
    }
    mini = ns(Monster_test(monster));
    if (!mini) {
        printf("test monster not there\n");
        return -1;
    }
    if (strcmp(ns(Monster_name(mini)), "Fred")) {
        printf("test monster isn't Fred\n");
        return -1;
    }
    testvec = ns(Monster_test4(monster));
    if (ns(Test_vec_len(testvec)) != 2) {
        printf("Test struct vector has wrong length\n");
        return -1;
    }
    test = ns(Test_vec_at(testvec, 0));
    if (ns(Test_a(test) != 10)) {
        printf("Testvec[0].a is wrong\n");
        return -1;
    }
    if (ns(Test_b(test) != 20)) {
        printf("Testvec[0].b is wrong\n");
        return -1;
    }
    test = ns(Test_vec_at(testvec, 1));
    if (ns(Test_a(test) != 30)) {
        printf("Testvec[1].a is wrong\n");
        return -1;
    }
    if (ns(Test_b(test) != 40)) {
        printf("Testvec[1].b is wrong\n");
        return -1;
    }
    assert(ns(Monster_testhashs32_fnv1(monster)) == -579221183L);
    assert(ns(Monster_testhashu32_fnv1(monster)) == 3715746113L);
    assert(ns(Monster_testhashs64_fnv1(monster)) == 7930699090847568257LL);
    assert(ns(Monster_testhashu64_fnv1(monster)) == 7930699090847568257LL);
    assert(ns(Monster_testhashs32_fnv1a(monster)) == -1904106383L);
    assert(ns(Monster_testhashu32_fnv1a(monster)) ==  2390860913L);
    assert(ns(Monster_testhashs64_fnv1a(monster)) ==  4898026182817603057LL);
    assert(ns(Monster_testhashu64_fnv1a(monster)) ==  4898026182817603057LL);
    return 0;
}


/* We take arguments so test can run without copying sources. */
#define usage \
"wrong number of arguments:\n" \
"usage: <program> [<input-filename>]\n"

int main(int argc, char *argv[])
{
    int ret;
    size_t size;
    void *buffer, *raw_buffer;

    if (argc != 1 && argc != 2) {
        fprintf(stderr, usage);
        exit(1);
    }
    if (argc == 2) {
        filename = argv[1];
    }

    raw_buffer = readfile(filename, 1024, &size);
    buffer = aligned_alloc(256, align_up(256, size));
    memcpy(buffer, raw_buffer, size);
    free(raw_buffer);

    if (!buffer) {
        fprintf(stderr, "could not read binary test file: %s\n", filename);
        return -1;
    }
    hexdump("monsterdata_test.mon", buffer, size, stderr);
    /*
     * Not automated, but verifying size - 3 fails as expected because the last
     * object in the file is a string, and the zero termination fails.
     * size - 1 and size - 2 verifies because the buffers contains
     * padding. Note that `flatcc` does not pad at the end beyond whatever
     * is stored (normally a vtable), but this is generated with `flatc
     * v1.1`.
     */
    if (flatcc_verify_ok != ns(Monster_verify_as_root_with_identifier(buffer, size, "MONS"))) {
#if FLATBUFFERS_PROTOCOL_IS_BE
        fprintf(stderr, "flatc golden reference buffer was correctly rejected by flatcc verificiation\n"
                "because flatc is little endian and flatcc has been compiled for big endian protocol format\n");
        ret = 0;
        goto done;
#else
        fprintf(stderr, "could not verify foreign monster file\n");
        ret = -1;
        goto done;
#endif
    }

#if FLATBUFFERS_PROTOCOL_IS_BE
    fprintf(stderr, "flatcc compiled with big endian protocol failed to reject reference little endian buffer\n");
    ret = -1;
    goto done;
#else
    if (flatcc_verify_ok != ns(Monster_verify_as_root(buffer, size))) {
        fprintf(stderr, "could not verify foreign monster file with default identifier\n");
        ret = -1;
        goto done;
    }
    ret = verify_monster(buffer);
#endif

done:
    aligned_free(buffer);
    return ret;
}
