#include <stdio.h>

#include "monster_test_builder.h"
#include "monster_test_verifier.h"

#include "flatcc/support/hexdump.h"
#include "flatcc/support/elapsed.h"
#include "flatcc/portable/pparsefp.h"
#include "../../config/config.h"

/*
 * Convenience macro to deal with long namespace names,
 * and to make code reusable with other namespaces.
 *
 * Note: we could also use
 *
 *     #define ns(x) MyGame_Example_ ## x
 *
 * but it wouldn't doesn't handled nested ns calls.
 *
 * For historic reason some of this test does not use the ns macro
 * and some avoid nesting ns calls by placing parenthesis differently
 * although this isn't required with this wrapper macro.
 */
#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

#undef nsf
#define nsf(x) FLATBUFFERS_WRAP_NAMESPACE(Fantasy, x)

/*
 * Wrap the common namespace (flatbuffers_). Many operations in the
 * common namespace such as `flatbuffers_string_create` are also mapped
 * to member fields such as `MyGame_Example_Monster_name_create` and
 * this macro provides a consistent interface to namespaces with
 * `nsc(string_create)` similar to `ns(Monster_name_create)`.
 */
#undef nsc
#define nsc(x) FLATBUFFERS_WRAP_NAMESPACE(flatbuffers, x)

/* A helper to simplify creating buffers vectors from C-arrays. */
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

static const char zero_pad[100];

int verify_empty_monster(void *buffer)
{
    /* Proper id given. */
    ns(Monster_table_t) monster = ns(Monster_as_root_with_identifier)(buffer, ns(Monster_file_identifier));
    /* Invalid id. */
    ns(Monster_table_t) monster2 = ns(Monster_as_root_with_identifier(buffer, "1234"));
    /* `with_id` can also mean ignore id when given a null argument. */
    ns(Monster_table_t) monster3 = ns(Monster_as_root_with_identifier(buffer, 0));
    /* Excessive text in identifier is ignored. */
    ns(Monster_table_t) monster4 = ns(Monster_as_root_with_identifier(buffer, "MONSX"));
    /* Default id should match proper id. */
    ns(Monster_table_t) monster5 = ns(Monster_as_root(buffer));

    if (!monster) {
        printf("Monster not available\n");
        return -1;
    }
    if (monster2) {
        printf("Monster should not accept invalid identifier\n");
        return -1;
    }
    if (monster3 != monster) {
        printf("Monster should ignore identifier when given a null id\n");
        return -1;
    }
    if (monster4 != monster) {
        printf("Monster should accept a string as valid identifier");
        return -1;
    }
    if (monster5 != monster) {
        printf("Monster with default id should be accepted");
        return -1;
    }
    if (ns(Monster_hp(monster)) != 100) {
        printf("Health points are not as expected\n");
        return -1;
    }
    if (ns(Monster_hp_is_present(monster))) {
        printf("Health Points should default\n");
        return -1;
    }
    if (ns(Monster_pos_is_present(monster))) {
        printf("Position should be present\n");
        return -1;
    }
    if (ns(Monster_pos(monster)) != 0) {
        printf("Position shouldn't be available\n");
        return -1;
    }
    return 0;
}

int test_enums(flatcc_builder_t *B)
{
    (void)B;

    if (ns(neg_enum_neg1) != -12) {
        printf("neg_enum_neg1 should be -12, was %d\n", ns(neg_enum_neg1));
        return -1;
    }
    if (ns(neg_enum_neg2) != -11) {
        printf("neg_enum_neg1 should be -11, was %d\n", ns(neg_enum_neg2));
        return -1;
    }
    if (ns(int_enum_int1) != 2) {
        printf("int_enum_int1 should be 2\n");
        return -1;
    }
    if (ns(int_enum_int2) != 42) {
        printf("int_enum_int2 should be 42\n");
        return -1;
    }
    if (ns(hex_enum_hexneg) != -2) {
        printf("enum hexneg should be -2\n");
        return -1;
    }
    if (ns(hex_enum_hex1) != 3) {
        printf("hex_enum_hex1 should be 3\n");
        return -1;
    }
    if (ns(hex_enum_hex2) != INT32_C(0x7eafbeaf)) {
        printf("hex_enum_hex2 should be 0x7eafbeaf\n");
        return -1;
    }
    return 0;
}

int test_type_aliases(flatcc_builder_t *B)
{
    int ret = 0;
    void *buffer = 0;
    size_t size;
    ns(TypeAliases_table_t) ta;
    flatbuffers_uint8_vec_ref_t v8_ref;
    flatbuffers_double_vec_ref_t vf64_ref;

    flatcc_builder_reset(B);

    v8_ref = flatbuffers_uint8_vec_create(B, 0, 0);
    vf64_ref = flatbuffers_double_vec_create(B, 0, 0);
    ns(TypeAliases_create_as_root(B,
          INT8_MIN, UINT8_MAX, INT16_MIN, UINT16_MAX,
          INT32_MIN, UINT32_MAX, INT64_MIN, UINT64_MAX, 2.3f, 2.3, v8_ref, vf64_ref));
    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    if ((ret = ns(TypeAliases_verify_as_root(buffer, size)))) {

        hexdump("TypeAliases buffer", buffer, size, stderr);
        printf("could not verify TypeAliases table, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }
    ta = ns(TypeAliases_as_root(buffer));

    if (ns(TypeAliases_i8(ta)) != INT8_MIN) goto failed;
    if (ns(TypeAliases_i16(ta)) != INT16_MIN) goto failed;
    if (ns(TypeAliases_i32(ta)) != INT32_MIN) goto failed;
    if (ns(TypeAliases_i64(ta)) != INT64_MIN) goto failed;
    if (ns(TypeAliases_u8(ta)) != UINT8_MAX) goto failed;
    if (ns(TypeAliases_u16(ta)) != UINT16_MAX) goto failed;
    if (ns(TypeAliases_u32(ta)) != UINT32_MAX) goto failed;
    if (ns(TypeAliases_u64(ta)) != UINT64_MAX) goto failed;
    if (!parse_float_is_equal(ns(TypeAliases_f32(ta)), 2.3f)) goto failed;
    if (!parse_double_is_equal(ns(TypeAliases_f64(ta)), 2.3)) goto failed;
    if (sizeof(ns(TypeAliases_i8(ta))) != 1) goto failed;
    if (sizeof(ns(TypeAliases_i16(ta))) != 2) goto failed;
    if (sizeof(ns(TypeAliases_i32(ta))) != 4) goto failed;
    if (sizeof(ns(TypeAliases_i64(ta))) != 8) goto failed;
    if (sizeof(ns(TypeAliases_u8(ta))) != 1) goto failed;
    if (sizeof(ns(TypeAliases_u16(ta))) != 2) goto failed;
    if (sizeof(ns(TypeAliases_u32(ta))) != 4) goto failed;
    if (sizeof(ns(TypeAliases_u64(ta))) != 8) goto failed;
    if (sizeof(ns(TypeAliases_f32(ta))) != 4) goto failed;
    if (sizeof(ns(TypeAliases_f64(ta))) != 8) goto failed;

done:
    flatcc_builder_aligned_free(buffer);
    return ret;

failed:
    ret = -1;
    printf("Scalar type alias has unexpected value or size\n");
    goto done;
}

int test_empty_monster(flatcc_builder_t *B)
{
    int ret;
    ns(Monster_ref_t) root;
    void *buffer;
    size_t size;

    flatcc_builder_reset(B);

    flatbuffers_buffer_start(B, ns(Monster_file_identifier));
    ns(Monster_start(B));
    /* Cannot make monster empty as name is required. */
    ns(Monster_name_create_str(B, "MyMonster"));
    root = ns(Monster_end(B));
    flatbuffers_buffer_end(B, root);

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    hexdump("empty monster table", buffer, size, stderr);
    if ((ret = verify_empty_monster(buffer))) {
        goto done;
    }

    if ((ret = ns(Monster_verify_as_root_with_identifier(buffer, size, ns(Monster_file_identifier))))) {
        printf("could not verify empty monster, got %s\n", flatcc_verify_error_string(ret));
        return -1;
    }

    /*
     * Note: this will assert if the verifier is set to assert during
     * debugging. Also not that a buffer size - 1 is not necessarily
     * invalid, but because we pack vtables tight at the end, we expect
     * failure in this case.
     */
    if (flatcc_verify_ok == ns(Monster_verify_as_root(
                    buffer, size - 1))) {
        printf("Monster verify failed to detect short buffer\n");
        return -1;
    }

done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_typed_empty_monster(flatcc_builder_t *B)
{
    int ret = -1;
    ns(Monster_ref_t) root;
    void *buffer;
    size_t size;
    flatbuffers_fid_t fid = { 0 };

    flatcc_builder_reset(B);

    flatbuffers_buffer_start(B, ns(Monster_type_identifier));
    ns(Monster_start(B));
    /* Cannot make monster empty as name is required. */
    ns(Monster_name_create_str(B, "MyMonster"));
    root = ns(Monster_end(B));
    flatbuffers_buffer_end(B, root);


    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    hexdump("empty typed monster table", buffer, size, stderr);

    if (flatbuffers_get_type_hash(buffer) != flatbuffers_type_hash_from_name("MyGame.Example.Monster")) {

        printf("Monster does not have the expected type, got %lx\n", (unsigned long)flatbuffers_get_type_hash(buffer));
        goto done;
    }

    if (!flatbuffers_has_type_hash(buffer, ns(Monster_type_hash))) {
        printf("Monster does not have the expected type\n");
        goto done;
    }
    if (!flatbuffers_has_type_hash(buffer, 0x330ef481)) {
        printf("Monster does not have the expected type\n");
        goto done;
    }

    if (!verify_empty_monster(buffer)) {
        printf("typed empty monster should not verify with default identifier\n");
        goto done;
    }

    if ((ret = ns(Monster_verify_as_root_with_identifier(buffer, size, ns(Monster_type_identifier))))) {
        printf("could not verify typed empty monster, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    if ((ret = ns(Monster_verify_as_typed_root(buffer, size)))) {
        printf("could not verify typed empty monster, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    if ((ret = ns(Monster_verify_as_root_with_type_hash(buffer, size, ns(Monster_type_hash))))) {
        printf("could not verify empty monster with type hash, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    if ((ret = ns(Monster_verify_as_root_with_type_hash(buffer, size, flatbuffers_type_hash_from_name("MyGame.Example.Monster"))))) {
        printf("could not verify empty monster with explicit type hash, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    flatbuffers_identifier_from_type_hash(0x330ef481, fid);
    if ((ret = ns(Monster_verify_as_root_with_identifier(buffer, size, fid)))) {
        printf("could not verify typed empty monster, got %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    if (!ns(Monster_verify_as_root(buffer, size))) {
        printf("should not have verified with the original identifier since we use types\n");
        goto done;
    }
    ret = 0;

done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int verify_monster(void *buffer)
{
    ns(Monster_table_t) monster, mon, mon2;
    ns(Monster_vec_t) monsters;
    ns(Any_union_type_t) test_type;
    ns(Any_union_t) test_union;
    /* This is an encoded struct pointer. */
    ns(Vec3_struct_t) vec;
    const char *name;
    /* This is a more precise type as there is a length field prefix. */
    nsc(string_t) name2;
    /* This is a native struct type. */
    ns(Vec3_t) v;
    ns(Test_vec_t) testvec;
    ns(Test_t) testvec_data[] = {
        {0x10, 0x20}, {0x30, 0x40}, {0x50, 0x60}, {0x70, (int8_t)0x80}, {0x191, (int8_t)0x91}
    };
    ns(Test_struct_t) test;
    nsc(string_vec_t) strings;
    nsc(string_t) s;
    nsc(bool_vec_t) bools;
    ns(Stat_table_t) stat;
    int booldata[] = { 0, 1, 1, 0 };
    const uint8_t *inv;
    size_t i;

    if (!nsc(has_identifier(buffer, 0))) {
        printf("wrong monster identifier (when ignoring)\n");
        return -1;
    }
    if (!nsc(has_identifier(buffer, "MONS"))) {
        printf("wrong monster identifier (when explicit)\n");
        return -1;
    }
    if (!nsc(has_identifier(buffer, "MONSTER"))) {
        printf("extra characters in identifier should be ignored\n");
        return -1;
    }
    if (nsc(has_identifier(buffer, "MON1"))) {
        printf("accepted wrong monster identifier (when explicit)\n");
        return -1;
    }
    if (!nsc(has_identifier(buffer, ns(Monster_file_identifier)))) {
        printf("wrong monster identifier (via defined identifier)\n");
        return -1;
    }

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
    if ((size_t)vec & 15) {
        printf("Force align of Vec3 struct not correct\n");
    }
    /* -3.2f is actually -3.20000005 and not -3.2 due to representation loss.
     * For 32-bit GCC compilers, -3.2f might be another value, so use lower
     * precision portable comparison. */
    if (!parse_float_is_equal(ns(Vec3_z(vec)), -3.2f)) {
        printf("Position failing on z coordinate\n");
        return -1;
    }
    if (nsc(is_native_pe())) {
        if (!parse_float_is_equal(vec->x, 1.0f) ||
            !parse_float_is_equal(vec->y, 2.0f) ||
            !parse_float_is_equal(vec->z, -3.2f)) {
            printf("Position is incorrect\n");
            return -1;
        }
    }
    /*
     * NOTE: copy_from_pe and friends are provided in the builder
     * interface, not the read only interface, but for advanced uses
     * these may also be used for read operations.
     * Also note that if we want the target struct fully null padded
     * the struct must be zeroed first. The _clear operation is one way
     * to achieve this - but it is not required for normal read access.
     * See common_builder for more details. These operations can
     * actually be very useful in their own right, disregarding any
     * other flatbuffer logic when dealing with struct endian
     * conversions in other protocols.
     */
    ns(Vec3_clear(&v)); /* Not strictly needed here. */
    ns(Vec3_copy_from_pe(&v, vec));
    if (!parse_float_is_equal(v.x, 1.0f) ||
        !parse_float_is_equal(v.y, 2.0f) ||
        !parse_float_is_equal(v.z, -3.2f)) {
        printf("Position is incorrect after copy\n");
        return -1;
    }
    if (vec->test1 != 0 || vec->test1 != 0 ||
            memcmp(&vec->test3, zero_pad, sizeof(vec->test3)) != 0) {
        printf("Zero default not correct for struct\n");
        return -1;
    }
    name = ns(Monster_name(monster));
    if (!name || strcmp(name, "MyMonster")) {
        printf("Name is not correct\n");
        return -1;
    }
    name2 = ns(Monster_name(monster));
    if (nsc(string_len(name)) != 9 || nsc(string_len(name2)) != 9) {
        printf("Name length is not correct\n");
        return -1;
    }
    if (ns(Monster_color(monster)) != ns(Color_Green)) {
        printf("Monster isn't a green monster\n");
        return -1;
    }
    if (strcmp(ns(Color_name)(ns(Color_Green)), "Green")) {
        printf("Enum name map does not have a green solution\n");
        return -1;
    }
    /*
     * This is bit tricky because Color is a bit flag, so we can have
     * combinations that are expected, but that we do not know. The
     * known value logic does not accomodate for that.
     */
    if (!ns(Color_is_known_value(ns(Color_Green)))) {
        printf("Color enum does not recognize the value of the Green flag\n");
        return -1;
    }
    if (!ns(Color_is_known_value(1))) {
        printf("Color enum does not recognize the value of the Red flag\n");
        return -1;
    }
    if (ns(Color_is_known_value(4))) {
        printf("Color enum recognizes a value it shouldn't\n");
        return -1;
    }
    if (!ns(Color_is_known_value(8))) {
        printf("Color enum does not recognize the value of the Blue flag\n");
        return -1;
    }
    if (ns(Color_is_known_value(9))) {
        printf("Color enum recognizes a value it shouldn't\n");
        return -1;
    }
    if (!ns(Any_is_known_type(ns(Any_Monster)))) {
        printf("Any type does not accept Monster\n");
        return -1;
    }
    if (ns(Any_is_known_type(42))) {
        printf("Any type recognizes unexpected type\n");
        return -1;
    }
    inv = ns(Monster_inventory(monster));
    if ((nsc(uint8_vec_len(inv))) != 10) {
        printf("Inventory length unexpected\n");
        return -1;
    }
    for (i = 0; i < nsc(uint8_vec_len(inv)); ++i) {
        if (nsc(uint8_vec_at(inv, i)) != i) {
            printf("inventory item #%d is wrong\n", (int)i);
            return -1;
        }
    }
    if (ns(Monster_mana(monster) != 150)) {
        printf("Mana not default\n");
        return -1;
    }
    if (ns(Monster_mana_is_present(monster))) {
        printf("Mana should default\n");
        return -1;
    }
    if (!ns(Monster_hp_is_present(monster))) {
        printf("Health points should be present\n");
        return -1;
    }
    if (!ns(Monster_pos_is_present(monster))) {
        printf("Position should be present\n");
        return -1;
    }
    testvec = ns(Monster_test4(monster));
    if (ns(Test_vec_len(testvec)) != 5) {
        printf("Test4 vector is not the right length.\n");
        return -1;
    }
    /*
     * This particular test requires that the in-memory
     * array layout matches the array layout in the buffer.
     */
    if (flatbuffers_is_native_pe()) {
        for (i = 0; i < 5; ++i) {
            test = ns(Test_vec_at(testvec, i));
            if (testvec_data[i].a != ns(Test_a(test))) {
                printf("Test4 vec failed at index %d, member a\n", (int)i);
                return -1;
            }
            if (testvec_data[i].b != ns(Test_b(test))) {
                printf("Test4 vec failed at index %d, member a\n", (int)i);
                return -1;
            }
        }
    } else {
        printf("SKIPPING DIRECT VECTOR ACCESS WITH NON-NATIVE ENDIAN PROTOCOL\n");
    }
    monsters = ns(Monster_testarrayoftables(monster));
    if (ns(Monster_vec_len(monsters)) != 8) {
        printf("unexpected monster vector length\n");
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 5));
    assert(mon);
    name = ns(Monster_name(mon));
    if (strcmp(name, "TwoFace")) {
        printf("monster 5 isn't TwoFace");
        return -1;
    }
    mon2 = ns(Monster_vec_at(monsters, 1));
    if (mon2 != mon) {
        printf("DAG test failed, monster[5] != monster[1] as pointer\n");
        return -1;
    }
    name = ns(Monster_name(mon2));
    if (strcmp(name, "TwoFace")) {
        printf("monster 1 isn't Joker, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 2));
    name = ns(Monster_name(mon));
    if (strcmp(name, "Joker")) {
        printf("monster 2 isn't Joker, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 0));
    name = ns(Monster_name(mon));
    if (strcmp(name, "Gulliver")) {
        printf("monster 0 isn't Gulliver, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 3));
    name = ns(Monster_name(mon));
    if (strcmp(name, "TwoFace")) {
        printf("monster 3 isn't TwoFace, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 4));
    name = ns(Monster_name(mon));
    if (strcmp(name, "Joker")) {
        printf("monster 4 isn't Joker, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 6));
    name = ns(Monster_name(mon));
    if (strcmp(name, "Gulliver")) {
        printf("monster 6 isn't Gulliver, it is: %s\n", name);
        return -1;
    }
    mon = ns(Monster_vec_at(monsters, 7));
    name = ns(Monster_name(mon));
    if (strcmp(name, "Joker")) {
        printf("monster 7 isn't Gulliver, it is: %s\n", name);
        return -1;
    }
    strings = ns(Monster_testarrayofstring(monster));
    if (nsc(string_vec_len(strings) != 3)) {
        printf("Monster array of strings has wrong length\n");
        return -1;
    }
    if (strcmp(nsc(string_vec_at(strings, 0)), "Hello")) {
        printf("string elem 0 is wrong\n");
        return -1;
    }
    s = nsc(string_vec_at(strings, 1));
    if (nsc(string_len(s)) != 2) {
        printf("string 1 has wrong length");
        return -1;
    }
    if (memcmp(s, ",\0", 2)) {
        printf("string elem 1 has wrong content\n");
        return -1;
    }
    if (strcmp(nsc(string_vec_at(strings, 2)), "world!")) {
        printf("string elem 2 is wrong\n");
        return -1;
    }
    if (!ns(Monster_testarrayofbools_is_present(monster))) {
        printf("array of bools is missing\n");
        return -1;
    }
    bools = ns(Monster_testarrayofbools(monster));
    if (nsc(bool_vec_len(bools) != 4)) {
        printf("bools have wrong vector length\n");
        return -1;
    }
    if (sizeof(bools[0]) != 1) {
        printf("bools have wrong element size\n");
        return -1;
    }
    for (i = 0; i < 4; ++i) {
        if (nsc(bool_vec_at(bools, i) != booldata[i])) {
            printf("bools vector elem %d is wrong\n", (int)i);
            return -1;
        }
    }
    test_type = ns(Monster_test_type(monster));
    if (test_type != ns(Any_Monster)) {
        printf("the monster test type is not Any_Monster\n");
        return -1;
    }
    mon = ns(Monster_test(monster));
    if (strcmp(ns(Monster_name(mon)), "TwoFace")) {
        printf("the test monster is not TwoFace\n");
        return -1;
    }
    mon = ns(Monster_enemy(monster));
    if (strcmp(ns(Monster_name(mon)), "the enemy")) {
        printf("the monster is not the enemy\n");
        return -1;
    }
    if (ns(Monster_test_type(mon)) != ns(Any_NONE)) {
        printf("the enemy test type is not Any_NONE\n");
        return -1;
    }
    test_union = ns(Monster_test_union(monster));
    if (test_union.type != test_type) {
        printf("the monster test union type is not Any_Monster\n");
        return -1;
    }
    if (test_union.value != ns(Monster_test(monster))) {
        printf("the union monster has gone awol\n");
        return -1;
    }
    monsters = ns(Monster_testarrayoftables(mon));
    i = ns(Monster_vec_len(monsters));
    mon = ns(Monster_vec_at(monsters, i - 1));
    if (ns(Monster_test_type)(mon) != ns(Any_Monster)) {
        printf("The monster variant added with value, type methods is not working\n");
        return -1;
    }
    mon = ns(Monster_test(mon));
    if (strcmp(ns(Monster_name(mon)), "TwoFace")) {
        printf("The monster variant added with value method is incorrect\n");
        return -1;
    }
    if (ns(Monster_testbool(monster))) {
        printf("testbool should not\n");
        return -1;
    }
    if (!ns(Monster_testempty_is_present(monster))) {
        printf("The empty table isn't present\n");
        return -1;
    }
    stat = ns(Monster_testempty(monster));
    if (ns(Stat_id_is_present(stat))
            || ns(Stat_val_is_present(stat))
            || ns(Stat_count_is_present(stat))) {
        printf("empty table isn't empty\n");
        return -1;
    }
    return 0;
}

int gen_monster(flatcc_builder_t *B, int with_size)
{
    uint8_t inv[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    ns(Vec3_t) *vec;
    ns(Test_t) *test, x;
    ns(Monster_ref_t) mon, mon2, monsters[2];
    ns(Monster_ref_t) *aoft;
    nsc(string_ref_t) name;
    nsc(string_ref_t) strings[3];
    nsc(bool_t)bools[] = { 0, 1, 1, 0 };
    flatcc_builder_reset(B);



    /*
     * Some FlatBuffer language interfaces require a string and other
     * non-embeddable objects to be created before the table storing it
     * is being created. This is not necessary (but possible) here
     * because the flatcc_builder maintains an internal stack.
     */
    if (with_size) {
        ns(Monster_start_as_root_with_size(B));
    } else {
        ns(Monster_start_as_root(B));
    }

    ns(Monster_hp_add(B, 80));
    vec = ns(Monster_pos_start(B));
    vec->x = 1, vec->y = 2, vec->z = -3.2f;
    /* _end call converts to protocol endian format. */
    ns(Monster_pos_end(B));
    /*
     * NOTE: Monster_name_add requires a reference to an
     * already created string - adding a string directly
     * will compile with a warning but fail badly. Instead
     * create the string first, or do it in-place with
     * the helper function `Monster_name_create_str`, or
     * with one of several other options.
     *
     * Wrong: ns(Monster_name_add(B, "MyMonster"));
     */
    ns(Monster_name_create_str(B, "MyMonster"));

    ns(Monster_color_add)(B, ns(Color_Green));

    ns(Monster_inventory_create(B, inv, c_vec_len(inv)));

    /* The vector is built in native endian format. */
    ns(Monster_test4_start(B));
    test = ns(Monster_test4_extend(B, 1));
    test->a = 0x10;
    test->b = 0x20;
    test = ns(Monster_test4_extend(B, 2));
    test->a = 0x30;
    test->b = 0x40;
    test[1].a = 0x50;
    test[1].b = 0x60;
    ns(Monster_test4_push_create(B, 0x70, (int8_t)0x80));
    /*
     * Zero padding within struct
     * - not needed when receiving a pointer like `test` in the above.
     */
    ns(Test_clear(&x));
    x.a = 0x190; /* This is a short. */
    x.b = (int8_t)0x91; /* This is a byte. */
    /* And x also has a hidden trailing padding byte. */
    ns(Monster_test4_push(B, &x));
    ns(Monster_test4_push(B, &x));
    /* We can use either field mapped push or push on the type. */
    ns(Test_vec_push(B, &x));
    /*
     * `_reserved_len` is similar to the `_vec_len` function in the
     * reader interface but `_vec_len` would not work here.
     */
    assert(ns(Monster_test4_reserved_len(B)) == 7);
    ns(Monster_test4_truncate(B, 2));
    assert(ns(Monster_test4_reserved_len(B)) == 5);

    /* It is not valid to dereference old pointers unless we call edit first. */
    test = ns(Monster_test4_edit(B));
    test[4].a += 1; /* 0x191 */

    /* Each vector element is converted to protocol endian format at end. */
    ns(Monster_test4_end(B));

    /* Test creating object before containing vector. */
    ns(Monster_start(B));
    name = nsc(string_create(B, "TwoFace", 7));
    ns(Monster_name_add(B, name));
    mon = ns(Monster_end(B));
    /*
     * Here we create several monsters with only a name - this also
     * tests reuse of vtables.
     */
    ns(Monster_testarrayoftables_start(B));
    aoft = ns(Monster_testarrayoftables_extend(B, 2));
    /*
     * It is usually not ideal to update reference vectors directly and
     * there must not be any unassigned elements (null) when the array
     * ends. Normally a push_start ... push_end, or a push_create
     * operation is preferable.
     */
    aoft[0] = mon;
    /*
     * But we can do things not otherwise possible - like constructing a
     * DAG. Note that reference values (unlike pointers) are stable as
     * long as the buffer is open for write, also past this vector.
     */
    aoft[1] = mon;
    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_strn(B, "Joker", 30));
    mon2 = *ns(Monster_testarrayoftables_push_end(B));
    aoft = ns(Monster_testarrayoftables_extend(B, 3));
    aoft[0] = mon;
    aoft[1] = mon2;
    ns(Monster_testarrayoftables_truncate(B, 1));
    assert(ns(Monster_testarrayoftables_reserved_len(B)) == 5);
    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_strn(B, "Gulliver at the Big Endians", 8));
    /* We cannot call reserved_len while a monster is still open, */
    monsters[0] = *ns(Monster_testarrayoftables_push_end(B));
    /* but here the vector is on top of the stack again. */
    assert(ns(Monster_testarrayoftables_reserved_len(B)) == 6);
    /* Swap monsters[0] and monsters[5] */
    aoft = ns(Monster_testarrayoftables_edit(B));
    mon2 = aoft[5];
    monsters[1] = aoft[2];
    aoft[5] = mon;
    aoft[0] = mon2;
    ns(Monster_testarrayoftables_append(B, monsters, 2));
    /*
     * The end call converts the reference array into an endian encoded
     * offset vector.
     */
    ns(Monster_testarrayoftables_end(B));

    strings[0] = nsc(string_create_str(B, "Hello"));
    /* Test embedded null character.
     * Note _strn is at most n, or up to 0 termination:
     * wrong: strings[1] = nsc(string_create_strn(B, ",\0", 2));
     */
    strings[1] = nsc(string_create(B, ",\0", 2));
    strings[2] = nsc(string_create_str(B, "world!"));
    ns(Monster_testarrayofstring_create(B, strings, 3));

    assert(c_vec_len(bools) == 4);
    ns(Monster_testarrayofbools_start(B));
    ns(Monster_testarrayofbools_append(B, bools, 1));
    ns(Monster_testarrayofbools_append(B, bools + 1, 3));
    ns(Monster_testarrayofbools_end(B));

    /*
     * This is using a constructor argument list where a union
     * is a single argument, unlike the C++ interface.
     * A union is given a type and a table reference.
     *
     * We are not verifying the result as this is only to stress
     * the type system of the builder - except: the last array
     * element is tested to ensure add_value is getting through.
     */
    ns(Monster_test_add)(B, ns(Any_as_Monster(mon)));

    ns(Monster_enemy_start(B));
    ns(Monster_name_create_str(B, "the enemy"));

    /* Create array of monsters to test various union constructors. */
    ns(Monster_testarrayoftables_start(B));

    ns(Monster_vec_push_start(B));
    ns(Monster_test_add)(B, ns(Any_as_Monster(mon)));
    /* Name is required. */
    ns(Monster_name_create_str(B, "any name"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_test_Monster_add(B, mon));
    ns(Monster_name_create_str(B, "any name"));
    ns(Monster_vec_push_end(B));
    /*
     * `push_start`: We can use the field specific method, or the type specific method
     * that the field maps to.
     */
    ns(Monster_testarrayoftables_push_start(B));
    /*
     * This is mostly for internal use in create methods so the type
     * can be added last and pack better in the table.
     * `add_value` still takes union_ref because it is a NOP if
     * the union type is NONE.
     */
    ns(Monster_test_add_value(B, ns(Any_as_Monster(mon))));
    ns(Monster_name_create_str(B, "any name"));
    ns(Monster_test_add_type(B, ns(Any_Monster)));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_end(B));

    ns(Monster_enemy_end(B));

    ns(Monster_testbool_add(B, 0));

    ns(Monster_testempty_start(B));
    ns(Monster_testempty_end(B));

    ns(Monster_end_as_root(B));
    return 0;
}

int test_monster(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    int ret;

    gen_monster(B, 0);

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("monster table", buffer, size, stderr);
    if ((ret = ns(Monster_verify_as_root(buffer, size)))) {
        printf("Monster buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }
    ret = verify_monster(buffer);

    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_monster_with_size(flatcc_builder_t *B)
{
    void *buffer, *frame;
    size_t size, size2, esize;
    int ret;

    gen_monster(B, 1);

    frame = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("monster table with size", frame, size, stderr);
    if (((size_t)frame & 15)) {
        printf("Platform did not provide 16 byte aligned allocation and needs special attention.");
        printf("buffer address: %x\n", (flatbuffers_uoffset_t)(size_t)frame);
        return -1;
    }

    buffer = flatbuffers_read_size_prefix(frame, &size2);
    esize = size - sizeof(flatbuffers_uoffset_t);
    if (size2 != esize) {
        printf("Size prefix has unexpected size, got %i, expected %i\n", (int)size2, (int)esize);
        return -1;
    }
    if ((ret = ns(Monster_verify_as_root(buffer, size2)))) {
        printf("Monster buffer with size prefix failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }
    ret = verify_monster(buffer);

    flatcc_builder_aligned_free(frame);
    return ret;
}

int test_cloned_monster(flatcc_builder_t *B)
{
    void *buffer;
    void *cloned_buffer;
    size_t size;
    int ret;
    flatcc_refmap_t refmap, *refmap_old;

    flatcc_refmap_init(&refmap);
    gen_monster(B, 0);

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("monster table", buffer, size, stderr);
    if ((ret = ns(Monster_verify_as_root(buffer, size)))) {
        printf("Monster buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }
    if (verify_monster(buffer)) {
        return -1;
    }
    flatcc_builder_reset(B);

    /*
     * Clone works without setting a refmap - but then shared references
     * get expanded - and then the verify monster check fails on a DAG
     * test.
     */
    refmap_old = flatcc_builder_set_refmap(B, &refmap);
    if (!ns(Monster_clone_as_root(B, ns(Monster_as_root(buffer))))) {
        printf("Cloned Monster didn't actually clone.");
        return -1;
    };
    /*
     * Restoring old refmap (or zeroing) is optional if we cleared the
     * buffer in this scope, but we don't so we must detach and clean up
     * the refmap manually. refmap_old is likely just null, but this
     * way we do not interfere with caller.
     */
    flatcc_builder_set_refmap(B, refmap_old);
    cloned_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("cloned monster table", cloned_buffer, size, stderr);
    if ((ret = ns(Monster_verify_as_root(cloned_buffer, size)))) {
        printf("Cloned Monster buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }
    if (verify_monster(cloned_buffer)) {
        printf("Cloned Monster did not have the expected content.");
        return -1;
    }

    flatcc_refmap_clear(&refmap);
    flatcc_builder_aligned_free(buffer);
    flatcc_builder_aligned_free(cloned_buffer);
    return ret;
}

int test_string(flatcc_builder_t *B)
{
    ns(Monster_table_t) mon;
    void *buffer;
    char *s;

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_start(B));
    s = ns(Monster_name_extend(B, 3));
    s[0] = '1';
    s[1] = '2';
    s[2] = '3';
    ns(Monster_name_append_str(B, "4"));
    assert(ns(Monster_name_reserved_len(B)) == 4);
    ns(Monster_name_append_strn(B, "5678", 30));
    assert(ns(Monster_name_reserved_len(B)) == 8);
    ns(Monster_name_append(B, "90", 2));
    assert(ns(Monster_name_reserved_len(B)) == 10);
    ns(Monster_name_truncate(B, 3));
    assert(ns(Monster_name_reserved_len(B)) == 7);
    s = ns(Monster_name_edit(B));
    s[4] = '.';
    ns(Monster_name_end(B));
    ns(Monster_end_as_root(B));
    /* Only with small buffers and the default emitter. */
    buffer = flatcc_builder_get_direct_buffer(B, 0);
    assert(buffer);
    mon = ns(Monster_as_root(buffer));
    if (strcmp(ns(Monster_name(mon)), "1234.67")) {
        printf("string test failed\n");
        return -1;
    }
    return 0;
}

int test_sort_find(flatcc_builder_t *B)
{
    size_t pos;
    ns(Monster_table_t) mon;
    ns(Monster_vec_t) monsters;
    ns(Monster_mutable_vec_t) mutable_monsters;
    void *buffer;
    size_t size;
    int ret = -1;

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));

    ns(Monster_testarrayoftables_start(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "TwoFace"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Joker"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Gulliver"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Alice"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Gulliver"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_end(B));

    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    hexdump("unsorted monster buffer", buffer, size, stderr);
    mon = ns(Monster_as_root(buffer));
    monsters = ns(Monster_testarrayoftables(mon));
    assert(monsters);
    mutable_monsters = (ns(Monster_mutable_vec_t))monsters;
    ns(Monster_vec_sort_by_name(mutable_monsters));

    hexdump("sorted monster buffer", buffer, size, stderr);

    if (ns(Monster_vec_len(monsters)) != 5) {
        printf("Sorted monster vector has wrong length\n");
        goto done;
    }
    if (strcmp(ns(Monster_name(ns(Monster_vec_at(monsters, 0)))), "Alice")) {
        printf("sort isn't working at elem 0\n");
        goto done;
    }
    if (strcmp(ns(Monster_name(ns(Monster_vec_at(monsters, 1)))), "Gulliver")) {
        printf("sort isn't working at elem 1\n");
        goto done;
    }
    if (strcmp(ns(Monster_name(ns(Monster_vec_at(monsters, 2)))), "Gulliver")) {
        printf("sort isn't working at elem 2\n");
        goto done;
    }
    if (strcmp(ns(Monster_name(ns(Monster_vec_at(monsters, 3)))), "Joker")) {
        printf("sort isn't working at elem 3\n");
        goto done;
    }
    if (strcmp(ns(Monster_name(ns(Monster_vec_at(monsters, 4)))), "TwoFace")) {
        printf("sort isn't working at elem 4\n");
        goto done;
    }
    /*
     * The heap sort isn't stable, but it should keep all elements
     * unique. Note that we could still have identical objects if we
     * actually stored the same object twice in DAG structure.
     */
    if (ns(Monster_vec_at(monsters, 1)) == ns(Monster_vec_at(monsters, 2))) {
        printf("Two identical sort keys should not be identical objects (in this case)\n");
        goto done;
    }

    if (3 != ns(Monster_vec_find(monsters, "Joker"))) {
        printf("find by default key did not find the Joker\n");
        goto done;
    }
    if (3 != ns(Monster_vec_find_n(monsters, "Joker2", 5))) {
        printf("find by default key did not find the Joker with n\n");
        goto done;
    }
    /*
     * We can have multiple keys on a table or struct by naming the sort
     * and find operations.
     */
    if (3 != ns(Monster_vec_find_by_name(monsters, "Joker"))) {
        printf("find did not find the Joker\n");
        goto done;
    }
    if (3 != ns(Monster_vec_find_n_by_name(monsters, "Joker3", 5))) {
        printf("find did not find the Joker with n\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_find_by_name(monsters, "Jingle"))) {
        printf("not found not working\n");
        goto done;
    }
    if (0 != ns(Monster_vec_find_by_name(monsters, "Alice"))) {
        printf("Alice not found\n");
        goto done;
    }
    /*
     * The search, unlike sort, is stable and should return the first
     * index of repeated keys.
     */
    if (1 != (pos = ns(Monster_vec_find_by_name(monsters, "Gulliver")))) {
        printf("Gulliver not found\n");
        printf("got %d\n", (int)pos);
        goto done;
    }
    if (4 != (pos = ns(Monster_vec_find_by_name(monsters, "TwoFace")))) {
        printf("TwoFace not found\n");
        printf("got %d\n", (int)pos);
        goto done;
    }

    /*
     * Just make sure the default key has a sort method - it is the same
     * as sort_by_name for the monster schema.
     */
    ns(Monster_vec_sort(mutable_monsters));
    ret = 0;

done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

static size_t count_monsters(ns(Monster_vec_t) monsters, const char *name)
{
    size_t i;
    size_t count = 0;

    for (i = ns(Monster_vec_scan)(monsters, name);
         i != nsc(not_found);
         i = ns(Monster_vec_scan_ex)(monsters, i + 1, nsc(end), name)) {
        ++count;
    }

    return count;
}

int test_scan(flatcc_builder_t *B)
{
    size_t pos;
    ns(Monster_table_t) mon;
    ns(Monster_vec_t) monsters;
    nsc(uint8_vec_t) inv;
    nsc(string_vec_t) strings;
    void *buffer;
    size_t size;
    uint8_t invdata[] = { 6, 7, 1, 3, 4, 3, 2 };
    int ret = -1;

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_inventory_create(B, invdata, c_vec_len(invdata)));

    ns(Monster_testarrayofstring_start(B));
    ns(Monster_testarrayofstring_end(B));

    ns(Monster_testarrayoftables_start(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "TwoFace"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Joker"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Gulliver"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Alice"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_push_start(B));
    ns(Monster_name_create_str(B, "Gulliver"));
    ns(Monster_testarrayoftables_push_end(B));

    ns(Monster_testarrayoftables_end(B));

    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    mon = ns(Monster_as_root(buffer));
    monsters = ns(Monster_testarrayoftables(mon));
    assert(monsters);
    inv = ns(Monster_inventory(mon));
    assert(inv);
    strings = ns(Monster_testarrayofstring(mon));
    assert(strings);

    if (1 != ns(Monster_vec_scan(monsters, "Joker"))) {
        printf("scan_by did not find the Joker\n");
        goto done;
    }
    if (1 != ns(Monster_vec_rscan(monsters, "Joker"))) {
        printf("rscan_by did not find the Joker\n");
        goto done;
    }
    if (1 != ns(Monster_vec_scan_n(monsters, "Joker3", 5))) {
        printf("scan_by did not find the Joker with n\n");
        goto done;
    }
    if (1 != ns(Monster_vec_rscan_n(monsters, "Joker3", 5))) {
        printf("scan_by did not find the Joker with n\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 2, nsc(end), "Joker"))) {
        printf("scan_from found Joker past first occurence\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan(monsters, "Jingle"))) {
        printf("not found not working\n");
        goto done;
    }
    if (0 != ns(Monster_vec_scan(monsters, "TwoFace"))) {
        printf("TwoFace not found\n");
        goto done;
    }
    if (2 != ns(Monster_vec_scan_by_name(monsters, "Gulliver"))) {
        printf("Gulliver not found\n");
        goto done;
    }
    if (4 != ns(Monster_vec_rscan_by_name(monsters, "Gulliver"))) {
        printf("Gulliver not found\n");
        goto done;
    }
    if (4 != ns(Monster_vec_rscan_n_by_name(monsters, "Gulliver42", 8))) {
        printf("Gulliver not found with n\n");
        goto done;
    }
    if (2 != ns(Monster_vec_rscan_ex_n_by_name(monsters, 1, 3, "Gulliver42", 8))) {
        printf("Gulliver not found with n\n");
        goto done;
    }
    if (2 != ns(Monster_vec_scan_ex_by_name(monsters, 2, nsc(end), "Gulliver"))) {
        printf("Gulliver not found starting from Gulliver\n");
        goto done;
    }
    if (2 != ns(Monster_vec_scan_ex_n_by_name(monsters, 2, nsc(end), "Gulliver42", 8))) {
        printf("Gulliver not found starting from Gulliver\n");
        goto done;
    }
    if (4 != ns(Monster_vec_scan_ex_by_name(monsters, 3, nsc(end), "Gulliver"))) {
        printf("Another Gulliver not found\n");
        goto done;
    }

    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 1, 3, "Jingle"))) {
        printf("not found in subrange not working\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 1, 3, "TwoFace"))) {
        printf("subrange doesn't limit low bound\n");
        goto done;
    }
    if (1 != ns(Monster_vec_scan_ex(monsters, 1, 3, "Joker"))) {
        printf("scan in subrange did not find Joker\n");
        goto done;
    }
    if (2 != ns(Monster_vec_scan_ex_by_name(monsters, 1, 3, "Gulliver"))) {
        printf("scan in subrange did not find Gulliver\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex_by_name(monsters, 1, 3, "Alice"))) {
        printf("subrange doesn't limit upper bound in scan\n");
        goto done;
    }

    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, 1, 3, "Jingle"))) {
        printf("not found in subrange not working with rscan\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, 1, 3, "TwoFace"))) {
        printf("subrange doesn't limit lower bound in rscan\n");
        goto done;
    }
    if (1 != ns(Monster_vec_rscan_ex(monsters, 1, 3, "Joker"))) {
        printf("rscan in subrange did not find Joker\n");
        goto done;
    }
    if (2 != ns(Monster_vec_rscan_ex_by_name(monsters, 1, 3, "Gulliver"))) {
        printf("rscan in subrange did not find Gulliver\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_rscan_ex_by_name(monsters, 1, 3, "Alice"))) {
        printf("subrange doesn't limit upper bound in rscan\n");
        goto done;
    }

    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 0, 0, "TwoFace"))) {
        printf("TwoFace is found in empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 0, 0, "Joker"))) {
        printf("Joker is found in empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, 1, 1, "Joker"))) {
        printf("Joker is found in another empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_scan_ex(monsters, ns(Monster_vec_len(monsters)), nsc(end), "TwoFace"))) {
        printf("TwoFace is found in empty range in the end\n");
        goto done;
    }

    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, 0, 0, "TwoFace"))) {
        printf("TwoFace is found in empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, 0, 0, "Joker"))) {
        printf("Joker is found in empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, 1, 1, "Joker"))) {
        printf("Joker is found in another empty range\n");
        goto done;
    }
    if (nsc(not_found) != ns(Monster_vec_rscan_ex(monsters, ns(Monster_vec_len(monsters)), nsc(end), "TwoFace"))) {
        printf("TwoFace is found in empty range in the end\n");
        goto done;
    }

    if (1 != count_monsters(monsters, "Joker")) {
        printf("number of Jokers is not 1\n");
        goto done;
    }
    if (0 != count_monsters(monsters, "Jingle")) {
        printf("number of Jingles is not 0\n");
        goto done;
    }
    if (1 != count_monsters(monsters, "TwoFace")) {
        printf("number of TwoFace is not 1\n");
        goto done;
    }
    if (2 != count_monsters(monsters, "Gulliver")) {
        printf("number of Gullivers is not 2\n");
        goto done;
    }


    if (0 != (pos = nsc(uint8_vec_scan(inv, 6)))) {
        printf("scan not working on first item of inventory\n");
        goto done;
    }
    if (2 != (pos = nsc(uint8_vec_scan(inv, 1)))) {
        printf("scan not working on middle item of inventory\n");
        goto done;
    }
    if (nsc(not_found) != (pos = nsc(uint8_vec_scan_ex(inv, 3, nsc(end), 1)))) {
        printf("scan_ex(item+1) not working on middle item of inventory\n");
        goto done;
    }
    if (nsc(not_found) != (pos = nsc(uint8_vec_scan(inv, 5)))) {
        printf("scan not working for repeating item of inventory\n");
        goto done;
    }
    if (6 != (pos = nsc(uint8_vec_scan(inv, 2)))) {
        printf("scan not working on last item of inventory\n");
        goto done;
    }
    if (3 != (pos = nsc(uint8_vec_scan(inv, 3)))) {
        printf("scan not working for repeating item of inventory\n");
        goto done;
    }
    if (3 != (pos = nsc(uint8_vec_scan_ex(inv, 3, nsc(end), 3)))) {
        printf("scan_ex(item) not working for repeating item of inventory\n");
        goto done;
    }
    if (5 != (pos = nsc(uint8_vec_scan_ex(inv, 4, nsc(end), 3)))) {
        printf("scan_ex(item+1) not working for repeating item of inventory\n");
        goto done;
    }
    if (5 != (pos = nsc(uint8_vec_rscan(inv, 3)))) {
        printf("rscan not working for repeating item of inventory\n");
        goto done;
    }
    if (3 != (pos = nsc(uint8_vec_rscan_ex(inv, 1, 4, 3)))) {
        printf("rscan_ex not working for repeating item of inventory\n");
        goto done;
    }

    /* Test that all scan functions are generated for string arrays */
    nsc(string_vec_scan(strings, "Hello"));
    nsc(string_vec_scan_ex(strings, 0, nsc(end), "Hello"));
    nsc(string_vec_scan_n(strings, "Hello", 4));
    nsc(string_vec_scan_ex_n(strings, 0, nsc(end), "Hello", 4));
    nsc(string_vec_rscan(strings, "Hello"));
    nsc(string_vec_rscan_ex(strings, 0, nsc(end), "Hello"));
    nsc(string_vec_rscan_n(strings, "Hello", 4));
    nsc(string_vec_rscan_ex_n(strings, 0, nsc(end), "Hello", 4));

#if FLATCC_ALLOW_SCAN_FOR_ALL_FIELDS
    /* Check for presence of scan for non-key fields */
    ns(Monster_vec_scan_by_hp(monsters, 13));
    ns(Monster_vec_scan_ex_by_hp(monsters, 1, nsc(end), 42));
    ns(Monster_vec_rscan_by_hp(monsters, 1));
    ns(Monster_vec_rscan_ex_by_hp(monsters, 0, 2, 42));
#endif

    ret = 0;

done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_basic_sort(flatcc_builder_t *B)
{
    ns(Monster_table_t) mon;
    nsc(uint8_vec_t) inv;
    nsc(uint8_mutable_vec_t) minv;

    void *buffer;
    size_t size;
    uint8_t invdata[]       = { 6, 7, 1, 3, 4, 3, 2 };
    uint8_t sortedinvdata[] = { 1, 2, 3, 3, 4, 6, 7 };
    uint8_t v, i;

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_inventory_create(B, invdata, c_vec_len(invdata)));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);

    mon = ns(Monster_as_root(buffer));
    inv = ns(Monster_inventory(mon));
    minv = (nsc(uint8_mutable_vec_t))inv;
    nsc(uint8_vec_sort(minv));
    assert(nsc(uint8_vec_len(inv) == c_vec_len(invdata)));
    for (i = 0; i < nsc(uint8_vec_len(inv)); ++i) {
        v = nsc(uint8_vec_at(inv, i));
        if (v != sortedinvdata[i]) {
            printf("inventory not sorted\n");
            return -1;
        }
        if (nsc(uint8_vec_find(inv, v) != (i == 3 ? 2 : i))) {
            printf("find not working on inventory\n");
            return -1;
        }
    }
    return 0;
}

int test_clone_slice(flatcc_builder_t *B)
{
    ns(Monster_table_t) mon, mon2;
    nsc(string_vec_t) strings;
    nsc(bool_vec_t) bools;
    nsc(string_t) name;
    ns(Monster_ref_t) monster_ref;
    ns(Test_t) *t;
    ns(Test_struct_t) test4;
    ns(Test_struct_t) elem4;
    void *buffer, *buf2;
    size_t size;
    int ret = -1;
    uint8_t booldata[] = { 0, 1, 0, 0, 1, 0, 0 };

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "The Source"));
    ns(Monster_testarrayofbools_create(B, booldata, c_vec_len(booldata)));

    ns(Monster_test4_start(B));
    t = ns(Monster_test4_extend(B, 2));
    t[0].a = 22;
    t[1].a = 44;
    ns(Monster_test4_end(B));
    ns(Monster_pos_start(B))->x = -42.3f;

    ns(Monster_end_as_root(B));
    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    hexdump("clone slice source buffer", buffer, size, stderr);

    mon = ns(Monster_as_root(buffer));

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));

    name = ns(Monster_name(mon));
    assert(name);
    bools = ns(Monster_testarrayofbools(mon));
    assert(bools);
    test4 = ns(Monster_test4(mon));
    assert(test4);

    ns(Monster_name_clone(B, name));
    ns(Monster_testarrayofstring_start(B));
    ns(Monster_testarrayofstring_push_clone(B, name));
    ns(Monster_testarrayofstring_push_slice(B, name, 4, 20));
    ns(Monster_testarrayofstring_push_slice(B, name, 0, 3));
    ns(Monster_testarrayofstring_end(B));
    ns(Monster_start(B));
    ns(Monster_name_slice(B, name, 2, 20));
    ns(Monster_testarrayofbools_clone(B, bools));

    ns(Monster_test4_slice(B, test4, 1, 2));


    monster_ref = ns(Monster_end(B));

    ns(Monster_test_add(B, ns(Any_as_Monster(monster_ref))));
    ns(Monster_testarrayofbools_slice(B, bools, 3, (size_t)-1));

    ns(Monster_pos_clone(B, ns(Monster_pos(mon))));
    ns(Monster_test4_clone(B, test4));

    ns(Monster_end_as_root(B));

    buf2 = flatcc_builder_get_direct_buffer(B, &size);
    hexdump("target buffer of clone", buf2, size, stderr);
    mon2 = ns(Monster_as_root(buf2));

    if (strcmp(ns(Monster_name(mon2)), "The Source")) {
        printf("The Source was not cloned\n");
        goto done;
    }

    strings = ns(Monster_testarrayofstring(mon2));
    if (strcmp(nsc(string_vec_at(strings, 0)), "The Source")) {
        printf("Push clone failed The Source\n");
        goto done;
    }
    if (nsc(string_len(nsc(string_vec_at(strings, 1)))) != 6) {
        printf("Push slice failed Sourcee on length\n");
        goto done;
    }
    if (strcmp(nsc(string_vec_at(strings, 1)), "Source")) {
        printf("Push slice failed Source\n");
        goto done;
    }
    if (nsc(string_len(nsc(string_vec_at(strings, 2)))) != 3) {
        printf("Push slice failed The on length\n");
        goto done;
    }
    if (strcmp(nsc(string_vec_at(strings, 2)), "The")) {
        printf("Push slice failed The\n");
        goto done;
    }
    mon = ns(Monster_test(mon2));
    assert(mon);
    if (strcmp(ns(Monster_name(mon)), "e Source")) {
        printf("name_slice did not shorten The Source correctly");
        goto done;
    }
    bools = ns(Monster_testarrayofbools(mon));
    if (nsc(bool_vec_len(bools)) != 7) {
        printf("clone bool has wrong length\n");
        goto done;
    }
    if (memcmp(bools, booldata, 7)) {
        printf("cloned bool has wrong content\n");
        goto done;
    }

    bools = ns(Monster_testarrayofbools(mon2));
    if (nsc(bool_vec_len(bools)) != 4) {
        printf("slice bool has wrong length\n");
        goto done;
    }
    if (memcmp(bools, booldata + 3, 4)) {
        printf("sliced bool has wrong content\n");
        goto done;
    }
    if (!parse_float_is_equal(ns(Monster_pos(mon2))->x, -42.3f)) {
        printf("cloned pos struct failed\n");
        goto done;
    };
    test4 = ns(Monster_test4(mon2));
    if (ns(Test_vec_len(test4)) != 2) {
        printf("struct vector test4 not cloned with correct length\n");
        goto done;
    }
    elem4 = ns(Test_vec_at(test4, 0));
    if (ns(Test_a(elem4)) != 22) {
        printf("elem 0 of test4 not cloned\n");
        goto done;
    }
    if (flatbuffers_is_native_pe() && ns(Test_vec_at(test4, 0))->a != 22) {
        printf("elem 0 of test4 not cloned, direct access\n");
        goto done;
    }
    elem4 = ns(Test_vec_at(test4, 1));
    if (ns(Test_a(elem4)) != 44) {
        printf("elem 1 of test4 not cloned\n");
        goto done;
    }
    test4 = ns(Monster_test4(mon));
    if (ns(Test_vec_len(test4)) != 1) {
        printf("sliced struct vec not sliced\n");
        goto done;
    }
    elem4 = ns(Test_vec_at(test4, 0));
    if (ns(Test_a(elem4)) != 44) {
        printf("sliced struct vec has wrong element\n");
        goto done;
    }

    /*
     * There is no push clone of structs because it becomes messy when
     * the vector has to be ended using end_pe or alternative do double
     * conversion with unclear semantics.
     */

    ret = 0;

done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_create_add_field(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    int ret = -1;
    ns(Monster_table_t) mon;
    ns(Stat_table_t) stat;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_testempty_create(B, nsc(string_create_str(B, "hello")), -100, 2));
    ns(Monster_enemy_add(B, 0));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    mon = ns(Monster_as_root(buffer));
    if (ns(Monster_enemy_is_present(mon))) {
        printf("enemy should not be present when adding null\n");
        goto done;
    }
    stat = ns(Monster_testempty(mon));
    if (!(ns(Stat_val(stat)) == -100)) {
        printf("Stat didn't happen\n");
        goto done;
    }
    ret = 0;
done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int verify_union_vector(void *buffer, size_t size)
{
    int ret = -1;
    size_t n;
    int color;

    ns(Monster_table_t) mon;
    ns(TestSimpleTableWithEnum_table_t) kermit;
    flatbuffers_generic_vec_t anyvec;
    ns(Any_vec_t) anyvec_type;
    ns(Any_union_vec_t) anyvec_union;
    ns(Any_union_t) anyelem;
    ns(Alt_table_t) alt;

    if ((ret = ns(Monster_verify_as_root(buffer, size)))) {
        printf("Monster buffer with union vector failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        goto failed;
    }

    mon = ns(Monster_as_root(buffer));
    if (ns(Monster_test_type(mon)) != ns(Any_Alt)) {
        printf("test field does not have Alt type");
        goto failed;
    }
    alt = ns(Monster_test(mon));
    if (!alt || !ns(Alt_manyany_is_present(alt))) {
        printf("manyany union vector should be present.\n");
        goto failed;
    }
    anyvec_type = ns(Alt_manyany_type(alt));
    anyvec = ns(Alt_manyany(alt));
    n = ns(Any_vec_len(anyvec_type));
    if (n != 1) {
        printf("manyany union vector has wrong length.\n");
        goto failed;
    }
    if (nsc(union_type_vec_at(anyvec_type, 0)) != ns(Any_TestSimpleTableWithEnum)) {
        printf("manyany union vector has wrong element type.\n");
        goto failed;
    }
    kermit = flatbuffers_generic_vec_at(anyvec, 0);
    if (!kermit) {
        printf("Kermit is lost.\n");
        goto failed;
    }
    color = ns(TestSimpleTableWithEnum_color(kermit));
    if (color != ns(Color_Green)) {
        printf("Kermit has wrong color: %i.\n", (int)color);
        goto failed;
    }
    anyvec_union = ns(Alt_manyany_union(alt));
    if (ns(Any_union_vec_len(anyvec_union)) != 1) {
        printf("manyany union vector has wrong length from a different perspective.\n");
        goto failed;
    }
    anyelem = ns(Any_union_vec_at(anyvec_union, 0));
    if (anyelem.type != nsc(union_type_vec_at(anyvec_type, 0))) {
        printf("Kermit is now different.\n");
        goto failed;
    }
    if (anyelem.value != kermit) {
        printf("Kermit is incoherent.\n");
        goto failed;
    }
    ret = 0;

done:
    return ret;

failed:
    ret = -1;
    goto done;
}

int test_union_vector(flatcc_builder_t *B)
{
    void *buffer = 0, *cloned_buffer = 0;
    size_t size;
    int ret = -1;
    flatcc_refmap_t refmap, *refmap_old;

    ns(TestSimpleTableWithEnum_ref_t) kermit_ref;
    ns(Any_union_vec_ref_t) anyvec_ref;


    flatcc_refmap_init(&refmap);
    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "Kermit"));

    kermit_ref = ns(TestSimpleTableWithEnum_create(B,
                ns(Color_Green), ns(Color_Green),
                ns(Color_Green), ns(Color_Green)));
    ns(Any_vec_start(B));
    ns(Any_vec_push(B, ns(Any_as_TestSimpleTableWithEnum(kermit_ref))));
    anyvec_ref = ns(Any_vec_end(B));
    ns(Monster_test_Alt_start(B));
    ns(Alt_manyany_add(B, anyvec_ref));
    ns(Monster_test_Alt_end(B));

    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    if (verify_union_vector(buffer, size)) {
        printf("Union vector Monster didn't verify.\n");
        goto failed;
    }
    flatcc_builder_reset(B);
    refmap_old = flatcc_builder_set_refmap(B, &refmap);
    if (!ns(Monster_clone_as_root(B, ns(Monster_as_root(buffer))))) {
        printf("Cloned union vector Monster didn't actually clone.\n");
        goto failed;
    };
    flatcc_builder_set_refmap(B, refmap_old);
    cloned_buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    if (verify_union_vector(buffer, size)) {
        printf("Cloned union vector Monster didn't verify.\n");
        goto failed;
    }

    ret = 0;

done:
    flatcc_refmap_clear(&refmap);
    flatcc_builder_aligned_free(buffer);
    flatcc_builder_aligned_free(cloned_buffer);
    return ret;

failed:
    ret = -1;
    goto done;
}

int verify_fixed_length_array(const void *buffer, size_t size)
{
    const char *text;
    ns(Monster_table_t) mon;
    ns(Alt_table_t) alt;
    ns(FooBar_struct_t) fa;
    ns(FooBar_t) fa2;
    ns(Test_struct_t) t0, t1;
    int ret;

    if ((ret = ns(Monster_verify_as_root(buffer, size)))) {
        printf("Monster buffer with fixed length arrays failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }

    mon = ns(Monster_as_root(buffer));
    if (ns(Monster_test_type(mon)) != ns(Any_Alt)) {
        printf("test field does not have Alt type");
        return -1;
    }

    alt = ns(Monster_test(mon));
    if (!alt || !ns(Alt_fixed_array_is_present(alt))) {
        printf("fixed array should be present.\n");
        return -1;
    }

    fa = ns(Alt_fixed_array(alt));

    if (ns(FooBar_foo(fa, 0)) != 1.0f || ns(FooBar_bar(fa, 9) != 1000)) {
        printf("Monster buffer with fixed length arrays has wrong content\n");
        return -1;
    }

    if (ns(FooBar_foo_get(fa, 0)) != 1.0f || ns(FooBar_bar_get(fa, 9) != 1000)) {
        printf("Monster buffer with fixed length arrays has wrong content\n");
        return -1;
    }
    if (ns(FooBar_foo_get(fa, 16)) != 0.0f || ns(FooBar_bar_get(fa, 10) != 0)) {
        printf("Monster buffer with fixed length arrays has bad bounds check\n");
        return -1;
    }
    if (ns(FooBar_col_get(fa, 2)) != ns(Color_Red)) {
        printf("Fixed length enum array content not correct\n");
        return -1;
    }
    t0 = ns(FooBar_tests_get(fa, 0));
    t1 = ns(FooBar_tests_get(fa, 1));
    if (!t0 || !t1) {
        printf("Monster buffer with fixed length struct arrays has missing element\n");
        return -1;
    }
    if (ns(Test_a_get(t0)) != 0 || ns(Test_b_get(t0)) != 4) {
        printf("Monster buffer with fixed length struct arrays has wrong first element member content\n");
        return -1;
    }
    if (ns(Test_a_get(t1)) != 1 || ns(Test_b_get(t1)) != 2) {
        printf("Monster buffer with fixed length struct arrays has wrong second element member content\n");
        return -1;
    }

    /* Endian safe because char arrays are endian neutral. */
    text = ns(FooBar_text_get_ptr(fa));
    if (strncmp(text, "hello", ns(FooBar_text_get_len())) != 0) {
        printf("Monster buffer with fixed length array field has wrong text\n");
        return -1;
    }

    /*
     * Note: use ns(FooBar_foo_get_ptr(fa) to get a raw pointer to the
     * array is not endian safe. Since this is a struct array field,
     * fa->foo would also provide the raw pointer.
     */
    if (flatbuffers_is_native_pe()) {
        if (ns(FooBar_foo_get_ptr(fa))[1] != 2.0f) {
            printf("Monster buffer with fixed length arrays get_ptr has wrong content\n");
            return -1;
        }
    }

    ns(FooBar_copy_from_pe(&fa2, fa));
    if (fa2.foo[0] != 1.0f || fa2.foo[1] != 2.0f || fa2.foo[15] != 16.0f ||
            fa2.bar[0] != 100 || fa2.bar[9] != 1000) {
        printf("Monster buffer with copied fixed length arrays has wrong content\n");
        return -1;
    }
    if (fa2.foo[2] != 0.0f || fa2.foo[14] != 0.0f || fa2.bar[1] != 0 || fa2.bar[8] != 0) {
        printf("Monster buffer with copied fixed length arrays has not been zero padded\n");
        return -1;
    }

    /*
     * In-place conversion - a nop on little endian platforms.
     * Cast needed to remove const
     */
    ns(FooBar_from_pe)((ns(FooBar_t) *)fa);
    if (fa->foo[0] != 1.0f || fa->foo[1] != 2.0f || fa->foo[15] != 16.0f ||
            fa->bar[0] != 100 || fa->bar[9] != 1000) {
        printf("Monster buffer with in-place converted fixed length arrays has wrong content\n");
        return -1;
    }
    if (fa->foo[2] != 0.0f || fa->foo[14] != 0.0f || fa->bar[1] != 0 || fa->bar[8] != 0) {
        printf("Monster buffer with in-place converted fixed length arrays has not been zero padded\n");
        return -1;
    }
    return 0;
}

int test_fixed_length_array(flatcc_builder_t *B)
{
    void *buffer = 0;
    size_t size;
    int ret = -1;
    float foo_input[16] = { 1.0f, 2.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16.0f };
    int bar_input[10] = { 100, 0, 0, 0, 0, 0, 0, 0, 0, 1000 };
    ns(Color_enum_t) col_input[3] = { 0, 0, ns(Color_Red) };
    ns(Test_t) tests_input[2] = {{ 0, 4 }, { 1, 2 }};

    ns(FooBar_t) *foobar;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "Monolith"));
    ns(Monster_test_Alt_start(B));
    foobar = ns(Alt_fixed_array_start(B));
    foobar->foo[0] = 1.0f;
    foobar->foo[1] = 2.0f;
    foobar->foo[15] = 16.0f;
    foobar->bar[0] = 100;
    foobar->bar[9] = 1000;
    foobar->col[2] = ns(Color_Red);
    foobar->tests[0].b = 4;
    foobar->tests[1].a = 1;
    foobar->tests[1].b = 2;
    strncpy(foobar->text, "hello, world", ns(FooBar_text_get_len()));
    // or strncopy(foobar->text, "hello, world", sizeof(foobar->text));
    ns(Alt_fixed_array_end(B));
    ns(Monster_test_Alt_end(B));

    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    ret = verify_fixed_length_array(buffer, size);
    flatcc_builder_aligned_free(buffer);
    if (ret) return -1;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "Monolith"));
    ns(Monster_test_Alt_start(B));
    foobar = ns(Alt_fixed_array_start(B));
    ns(FooBar_assign)(foobar, foo_input, bar_input, col_input, tests_input, "hello");
    ns(Alt_fixed_array_end(B));
    ns(Monster_test_Alt_end(B));

    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    ret = verify_fixed_length_array(buffer, size);
    flatcc_builder_aligned_free(buffer);
    if (ret) return -1;

    return 0;
}

#define STR(s) nsc(string_create_str(B, s))

int test_recursive_sort(flatcc_builder_t *B)
{
    nsc(string_ref_t) name;

    void *buffer = 0;
    size_t size = 0;
    int ret = -1;
    ns(Alt_table_t) alt;
    ns(Any_union_t) any;
    ns(Monster_table_t) monster;
    ns(MultipleKeys_vec_t) mkvec;
    ns(MultipleKeys_table_t) mk;
    size_t index;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));

    name = STR("Keyed Monster");
    ns(Alt_start(B));
    ns(Alt_multik_start(B));
    ns(Alt_multik_push_create(B, STR("hi"), STR("there"), 42));
    ns(Alt_multik_push_create(B, STR("hello"), STR("anyone"), 10));
    ns(Alt_multik_push_create(B, STR("hello"), STR("anyone"), 4));
    ns(Alt_multik_push_create(B, STR("good day"), STR("sir"), 1004));
    ns(Alt_multik_end(B));
    ns(Monster_test_add)(B, ns(Any_as_Alt(ns(Alt_end(B)))));
    ns(Monster_name_add)(B, name);
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);
    monster = ns(Monster_as_root)(buffer);
    ns(Monster_sort)((ns(Monster_mutable_table_t))monster);
    any = ns(Monster_test_union(monster));
    if (any.type != ns(Any_Alt)) {
        printf("Any type no Alt as expected\n");
        goto done;
    }
    alt = any.value;
    mkvec = ns(Alt_multik(alt));
    index = ns(MultipleKeys_vec_len(mkvec));
    if (index != 4) {
        printf("unexpected multik vec len, got %d\n", (int)index);
        goto done;
    }
    mk = ns(MultipleKeys_vec_at(mkvec, 0));
    if (ns(MultipleKeys_foobar(mk) != 4)) {
        printf("multik elem 0 not sorted, but it really should be\n");
    }
    mk = ns(MultipleKeys_vec_at(mkvec, 1));
    if (ns(MultipleKeys_foobar(mk) != 10)) {
        printf("multik elem 1 not sorted, but it really should be\n");
    }
    mk = ns(MultipleKeys_vec_at(mkvec, 2));
    if (ns(MultipleKeys_foobar(mk) != 42)) {
        printf("multik elem 2 not sorted, but it really should be\n");
    }
    mk = ns(MultipleKeys_vec_at(mkvec, 3));
    if (ns(MultipleKeys_foobar(mk) != 1004)) {
        printf("multik elem 3 not sorted, but it really should be\n");
    }

    hexdump("MultiKeyed buffer", buffer, size, stderr);
    if ((ret = ns(Monster_verify_as_root(buffer, size)))) {
        printf("Multikeyed Monster buffer failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        goto done;
    }

    ret = 0;
done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_mixed_type_union(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    int ret = -1;
    /* Builder */
    nsf(Character_union_ref_t) ut;
    nsf(Rapunzel_ref_t) cameo_ref;
    nsf(Attacker_ref_t) attacker_ref;
    nsf(BookReader_ref_t) br_ref;
    nsf(BookReader_t *) pbr;
    nsf(Movie_table_t) mov;

    /* Reader */
    nsf(Character_union_vec_t) characters;
    nsf(Character_union_t) character;
    nsf(Rapunzel_struct_t) rapunzel;
    nsf(Attacker_table_t) attacker;
    nsc(string_t) text;

    flatcc_builder_reset(B);

    nsf(Movie_start_as_root(B));
    br_ref = nsf(BookReader_create(B, 10));
    cameo_ref = nsf(Rapunzel_create(B, 22));
    ut = nsf(Character_as_Rapunzel(cameo_ref));
    nsf(Movie_main_character_Rapunzel_create(B, 19));
    nsf(Movie_cameo_Rapunzel_add(B, cameo_ref));
    attacker_ref = nsf(Attacker_create(B, 42));
    nsf(Movie_antagonist_MuLan_add(B, attacker_ref));
    nsf(Movie_side_kick_Other_create_str(B, "Nemo"));
    nsf(Movie_characters_start(B));
    nsf(Movie_characters_push(B, ut));
    nsf(Movie_characters_MuLan_push(B, attacker_ref));
    nsf(Movie_characters_MuLan_push_create(B, 1));
    nsf(Character_vec_push(B, nsf(Character_as_Other(nsc(string_create_str(B, "other"))))));
    nsf(Movie_characters_Belle_push(B, br_ref));
    pbr = nsf(Movie_characters_Belle_push_start(B));
    pbr->books_read = 3;
    nsf(Movie_characters_Belle_push_end(B));
    nsf(Movie_characters_Belle_push(B, nsf(BookReader_create(B, 1))));
    nsf(Movie_characters_Belle_push_create(B, 2));
    nsf(Movie_characters_Other_push(B, nsc(string_create_str(B, "another"))));
    nsf(Movie_characters_Other_push_create_str(B, "yet another"));
    nsf(Movie_characters_end(B));
    nsf(Movie_end_as_root(B));

    buffer = flatcc_builder_finalize_aligned_buffer(B, &size);

    hexdump("Movie buffer", buffer, size, stderr);
    if ((ret = nsf(Movie_verify_as_root(buffer, size)))) {
        printf("Movie buffer with mixed type union and union vector failed to verify, got: %s\n", flatcc_verify_error_string(ret));
        return -1;
    }
    ret = -1;

    mov = nsf(Movie_as_root(buffer));
    if (!nsf(Movie_main_character_is_present(mov))) {
        printf("Main_charactery union should be present.\n");
        goto done;
    }
    if (!nsf(Movie_characters_is_present(mov))) {
        printf("Characters union vector should be present.\n");
        goto done;
    }
    character = nsf(Movie_main_character_union(mov));
    if (character.type != nsf(Character_Rapunzel)) {
        printf("Unexpected main character.\n");
        goto done;
    };
    /*
     * Tables and structs can cast by void pointer assignment while
     * strings require an explicit cast.
     */
    rapunzel = character.value;
    if (!rapunzel) {
        printf("Rapunzel has gone AWOL\n");
    }
    if (nsf(Rapunzel_hair_length(rapunzel)) > 19) {
        printf("Rapunzel's hair has grown unexpectedly\n");
        goto done;
    }
    if (nsf(Rapunzel_hair_length(rapunzel)) < 19) {
        printf("Rapunzel's hair has been trimmed unexpectedly\n");
        goto done;
    }
    if (nsf(Movie_cameo_type(mov)) != nsf(Character_Rapunzel)) {
        printf("Rapunzel did was not selected for cameo appearance.\n");
        goto done;
    }
    rapunzel = nsf(Movie_cameo(mov));
    if (!rapunzel) {
        printf("Rapunzel did not show up for cameo appearance.\n");
        goto done;
    }
    if (nsf(Rapunzel_hair_length(rapunzel)) != 22) {
        printf("Rapunzel didn't style her hair for cameo role.\n");
        goto done;
    }
    if (nsf(Movie_antagonist_type(mov)) != nsf(Character_MuLan)) {
        printf("Unexpected antagonist.\n");
        goto done;
    }
    attacker = nsf(Movie_antagonist(mov));
    if (!attacker || nsf(Attacker_sword_attack_damage(attacker)) != 42) {
        printf("Unexpected sword attack damamage.\n");
        goto done;
    }
    if (nsf(Movie_side_kick_type(mov)) != nsf(Character_Other)) {
        printf("Unexpected side kick.\n");
        goto done;
    }
    /*
     * We need to cast because generic pointers refer to the start
     * of the memory block which is the string length, not the first
     * character in the string.
     */
    text = nsc(string_cast_from_generic(nsf(Movie_side_kick(mov))));
    if (!text) {
        printf("missing side kick string.\n");
        goto done;
    }
    if (strcmp(text, "Nemo")) {
        printf("unexpected side kick string: '%s'.\n", text);
        goto done;
    }
    text = nsf(Movie_side_kick_as_string(mov));
    if (!text) {
        printf("missing side kick string.\n");
        goto done;
    }
    if (strcmp(text, "Nemo")) {
        printf("unexpected side kick string (take 2): '%s'.\n", text);
        goto done;
    }
    character = nsf(Movie_side_kick_union(mov));
    text = nsc(string_cast_from_union(character));
    if (strcmp(text, "Nemo")) {
        printf("unexpected side kick string (take 3): '%s'.\n", text);
        goto done;
    }
    characters = nsf(Movie_characters_union(mov));
    character = nsf(Character_union_vec_at(characters, 0));
    if (character.type != nsf(Character_Rapunzel)) {
        printf("The first character is not Rapunzel.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 1));
    if (character.type != nsf(Character_MuLan)) {
        printf("The second character is not MuLan.");
        goto done;
    };
    attacker = character.value;
    if (nsf(Attacker_sword_attack_damage(attacker) != 42)) {
        printf("The second character has unexpected sword damage.");
        goto done;
    }
    character = nsf(Character_union_vec_at(characters, 2));
    if (character.type != nsf(Character_MuLan)) {
        printf("The third character is not MuLan.");
        goto done;
    };
    attacker = character.value;
    if (nsf(Attacker_sword_attack_damage(attacker) != 1)) {
        printf("The third character has unexpected sword damage.");
        goto done;
    }
    if (nsc(union_type_vec_at(nsf(Movie_characters_type(mov)), 3)) != nsf(Character_Other)) {
        printf("The fourth character was not of type 'Other'!\n");
        goto done;
    }
    text = nsf(Character_union_vec_at_as_string(characters, 3));
    if (!text || strcmp(text, "other")) {
        printf("The fourth character was not described as 'other'.\n");
        goto done;
    }
    character = nsf(Character_union_vec_at(characters, 3));
    if (character.type != nsf(Character_Other)) {
        printf("The fourth character is not of type 'Other' (take two).");
        goto done;
    };
    text = nsc(string_cast_from_union(character));
    if (!text || strcmp(text, "other")) {
        printf("The fourth character was not described as 'other' (take two).\n");
        goto done;
    }
    character = nsf(Character_union_vec_at(characters, 4));
    if (character.type != nsf(Character_Belle)) {
        printf("The fifth character is not Belle.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 5));
    if (character.type != nsf(Character_Belle)) {
        printf("The sixth character is not Belle.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 6));
    if (character.type != nsf(Character_Belle)) {
        printf("The seventh character is not Belle.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 7));
    if (character.type != nsf(Character_Belle)) {
        printf("The eighth character is not Belle.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 8));
    if (character.type != nsf(Character_Other)) {
        printf("The ninth character is not of type 'Other'.");
        goto done;
    };
    character = nsf(Character_union_vec_at(characters, 9));
    if (character.type != nsf(Character_Other)) {
        printf("The ninth character is not of type 'Other'.");
        goto done;
    };
    if (nsf(Character_union_vec_len(characters) != 10)) {
        printf("The 11'th character should not exist.");
        goto done;
    };

    ret = 0;
done:
    flatcc_builder_aligned_free(buffer);
    return ret;
}

int test_add_set_defaults(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    ns(Monster_table_t) mon;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_hp_add(B, 100));
    ns(Monster_mana_add(B, 100));
    ns(Monster_color_add(B, ns(Color_Blue)));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);
    mon = ns(Monster_as_root(buffer));
    if (ns(Monster_hp_is_present(mon))) {
        printf("default should not be present for hp field\n");
        return -1;
    }
    if (!ns(Monster_mana_is_present(mon))) {
        printf("non-default should be present for mana field\n");
        return -1;
    }
    if (ns(Monster_color_is_present(mon))) {
        printf("default should not be present for color field\n");
        return -1;
    }

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_hp_force_add(B, 100));
    ns(Monster_mana_force_add(B, 100));
    ns(Monster_color_force_add(B, ns(Color_Blue)));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);
    mon = ns(Monster_as_root(buffer));
    if (!ns(Monster_hp_is_present(mon))) {
        printf("default should be present for hp field when forced\n");
        return -1;
    }
    if (!ns(Monster_mana_is_present(mon))) {
        printf("non-default should be present for mana field, also when forced\n");
        return -1;
    }
    if (!ns(Monster_color_is_present(mon))) {
        printf("default should be present for color field when forced\n");
        return -1;
    }

    return 0;
}

int test_nested_buffer(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    ns(Monster_table_t) mon, nested;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    /*
     * Note:
     *   ns(Monster_testnestedflatbuffer_start(B));
     * would start a raw ubyte array so we use start_as_root.
     */
    ns(Monster_testnestedflatbuffer_start_as_root(B));
    ns(Monster_name_create_str(B, "MyNestedMonster"));
    ns(Monster_testnestedflatbuffer_end_as_root(B));
    ns(Monster_hp_add(B, 10));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);
    hexdump("nested flatbuffer", buffer, size, stderr);

    mon = ns(Monster_as_root(buffer));
    if (strcmp(ns(Monster_name(mon)), "MyMonster")) {
        printf("got the wrong root monster\n");
        return -1;
    }
    /*
     * Note:
     *   nested = ns(Monster_testnestedflatbuffer(mon));
     * would return a raw ubyte vector not a monster.
     */
    nested = ns(Monster_testnestedflatbuffer_as_root(mon));

    if (ns(Monster_hp(mon)) != 10) {
        printf("health points wrong at root monster\n");
        return -1;
    }

    assert(ns(Monster_name(nested)));
    if (strcmp(ns(Monster_name(nested)), "MyNestedMonster")) {
        printf("got the wrong nested monster\n");
        return -1;
    }

    return 0;
}

int test_nested_buffer_first(flatcc_builder_t *B)
{
    void *buffer;
    size_t size;
    ns(Monster_table_t) mon, nested;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    /*
     * Note:
     *   ns(Monster_testnestedflatbuffer_start(B));
     * would start a raw ubyte array so we use start_as_root.
     *
     * Here we create the nested buffer first, and the parent
     * string after so the emitter sees the nested buffer first.
     */
    ns(Monster_testnestedflatbuffer_start_as_root(B));
    ns(Monster_name_create_str(B, "MyNestedMonster"));
    ns(Monster_testnestedflatbuffer_end_as_root(B));
    ns(Monster_hp_add(B, 10));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);
    hexdump("nested flatbuffer", buffer, size, stderr);

    mon = ns(Monster_as_root(buffer));
    if (strcmp(ns(Monster_name(mon)), "MyMonster")) {
        printf("got the wrong root monster\n");
        return -1;
    }
    /*
     * Note:
     *   nested = ns(Monster_testnestedflatbuffer(mon));
     * would return a raw ubyte vector not a monster.
     */
    nested = ns(Monster_testnestedflatbuffer_as_root(mon));

    if (ns(Monster_hp(mon)) != 10) {
        printf("health points wrong at root monster\n");
        return -1;
    }

    assert(ns(Monster_name(nested)));
    if (strcmp(ns(Monster_name(nested)), "MyNestedMonster")) {
        printf("got the wrong nested monster\n");
        return -1;
    }

    return 0;
}

int test_nested_buffer_using_nest(flatcc_builder_t *B)
{
    void *buffer;
    uint8_t nested_buffer[1024];
    size_t size, nested_size;
    ns(Monster_table_t) mon, nested;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "MyNestedMonster"));
    ns(Monster_mana_add(B, 42));
    ns(Monster_end_as_root(B));

    nested_size = flatcc_builder_get_buffer_size(B);
    if (!flatcc_builder_copy_buffer(B, nested_buffer, sizeof(nested_buffer))) {
        printf("nested buffer copy failed\n");
        return -1;
    }

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_testnestedflatbuffer_nest(B, nested_buffer, nested_size, 0));
    ns(Monster_hp_add(B, 10));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_end_as_root(B));

    buffer = flatcc_builder_get_direct_buffer(B, &size);
    hexdump("nested flatbuffer [using _nest()]", buffer, size, stderr);

    mon = ns(Monster_as_root(buffer));
    if (strcmp(ns(Monster_name(mon)), "MyMonster")) {
        printf("got the wrong root monster\n");
        return -1;
    }
    /*
     * Note:
     *   nested = ns(Monster_testnestedflatbuffer(mon));
     * would return a raw ubyte vector not a monster.
     */
    nested = ns(Monster_testnestedflatbuffer_as_root(mon));

    if (ns(Monster_hp(mon)) != 10) {
        printf("health points wrong at root monster\n");
        return -1;
    }

    assert(ns(Monster_name(nested)));
    if (strcmp(ns(Monster_name(nested)), "MyNestedMonster")) {
        printf("got the wrong nested monster\n");
        return -1;
    }

    if (ns(Monster_mana(nested)) != 42) {
        printf("mana points wrong in nested monster\n");
        return -1;
    }

    return 0;
}

int verify_include(void *buffer)
{
    (void)buffer;

    if (MyGame_OtherNameSpace_FromInclude_Foo != 17) {
        printf("Unexpected enum value `Foo` from included schema\n");
        return -1;
    }

    if (MyGame_OtherNameSpace_FromInclude_IncludeVal != 0) {
        printf("Unexpected enum value `IncludeVal` from included schema\n");
        return -1;
    }

    return 0;
}


int test_struct_buffer(flatcc_builder_t *B)
{
    uint8_t buffer[100];

    size_t size;
    ns(Vec3_t) *v;
    ns(Vec3_struct_t) vec3;

    flatcc_builder_reset(B);
    ns(Vec3_create_as_root(B, 1, 2, 3, 4.2, ns(Color_Blue), 2730, -17));
    size = flatcc_builder_get_buffer_size(B);
    assert(size == 48);
    printf("dbg: struct buffer size: %d\n", (int)size);
    assert(flatcc_emitter_get_buffer_size(flatcc_builder_get_emit_context(B)) == size);
    if (!flatcc_builder_copy_buffer(B, buffer, 100)) {
        printf("Copy failed\n");
        return -1;
    }
    hexdump("Vec3 struct buffer", buffer, size, stderr);
    if (!nsc(has_identifier(buffer, "MONS"))) {
        printf("wrong Vec3 identifier (explicit)\n");
        return -1;
    }
    if (nsc(has_identifier(buffer, "mons"))) {
        printf("accepted wrong Vec3 identifier (explicit)\n");
        return -1;
    }
    if (!nsc(has_identifier(buffer, ns(Vec3_identifier)))) {
        printf("wrong Vec3 identifier (via define)\n");
        return -1;
    }
    vec3 = ns(Vec3_as_root(buffer));
    /* Convert buffer to native in place - a nop on native platform. */
    v = (ns(Vec3_t) *)vec3;
    ns(Vec3_from_pe(v));
    if (!parse_float_is_equal(v->x, 1.0f) || !parse_float_is_equal(v->y, 2.0f) || !parse_float_is_equal(v->z, 3.0f)
        || !parse_double_is_equal(v->test1, 4.2) || v->test2 != ns(Color_Blue)
        || v->test3.a != 2730 || v->test3.b != -17
       ) {
        printf("struct buffer not valid\n");
        return -1;
    }
    assert(ns(Color_Red) == 1 << 0);
    assert(ns(Color_Green) == 1 << 1);
    assert(ns(Color_Blue) == 1 << 3);
    assert(sizeof(ns(Color_Blue) == 1));
    return 0;
}

int test_typed_struct_buffer(flatcc_builder_t *B)
{
    uint8_t buffer[100];

    size_t size;
    ns(Vec3_t) *v;
    ns(Vec3_struct_t) vec3;

    flatcc_builder_reset(B);
    ns(Vec3_create_as_typed_root(B, 1, 2, 3, 4.2, ns(Color_Blue), 2730, -17));
    size = flatcc_builder_get_buffer_size(B);
    assert(size == 48);
    printf("dbg: struct buffer size: %d\n", (int)size);
    assert(flatcc_emitter_get_buffer_size(flatcc_builder_get_emit_context(B)) == size);
    if (!flatcc_builder_copy_buffer(B, buffer, 100)) {
        printf("Copy failed\n");
        return -1;
    }
    hexdump("typed Vec3 struct buffer", buffer, size, stderr);
    if (!nsc(has_identifier(buffer, "\xd2\x3e\xf5\xa8"))) {
        printf("wrong Vec3 identifier (explicit)\n");
        return -1;
    }
    if (nsc(has_identifier(buffer, "mons"))) {
        printf("accepted wrong Vec3 identifier (explicit)\n");
        return -1;
    }
    if (!nsc(has_identifier(buffer, ns(Vec3_type_identifier)))) {
        printf("wrong Vec3 identifier (via define)\n");
        return -1;
    }
    if (!ns(Vec3_as_root_with_type_hash(buffer, ns(Vec3_type_hash)))) {
        printf("wrong Vec3 type identifier (via define)\n");
        return -1;
    }
    if (flatcc_verify_ok != ns(Vec3_verify_as_root_with_type_hash(buffer, size, ns(Vec3_type_hash)))) {
        printf("verify failed with Vec3 type hash\n");
        return -1;
    }
    vec3 = ns(Vec3_as_typed_root(buffer));
    if (!vec3) {
        printf("typed Vec3 could not be read\n");
        return -1;
    }
    if (flatcc_verify_ok != ns(Vec3_verify_as_typed_root(buffer, size))) {
        printf("verify failed with Vec3 as typed root\n");
        return -1;
    }
    /* Convert buffer to native in place - a nop on native platform. */
    v = (ns(Vec3_t) *)vec3;
    ns(Vec3_from_pe(v));
    if (!parse_float_is_equal(v->x, 1.0f) || !parse_float_is_equal(v->y, 2.0f) || !parse_float_is_equal(v->z, 3.0f)
        || !parse_double_is_equal(v->test1, 4.2) || v->test2 != ns(Color_Blue)
        || v->test3.a != 2730 || v->test3.b != -17
       ) {
        printf("struct buffer not valid\n");
        return -1;
    }
    assert(ns(Color_Red) == 1 << 0);
    assert(ns(Color_Green) == 1 << 1);
    assert(ns(Color_Blue) == 1 << 3);
    assert(sizeof(ns(Color_Blue) == 1));
    return 0;
}


/* A stable test snapshot for reference. */
int gen_monster_benchmark(flatcc_builder_t *B)
{
    uint8_t inv[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    ns(Vec3_t) *vec;
    ns(Test_t) *test, x;

    flatcc_builder_reset(B);

    ns(Monster_start_as_root(B));
    ns(Monster_hp_add(B, 80));
    vec = ns(Monster_pos_start(B));
    vec->x = 1, vec->y = 2, vec->z = -3.2f;
    ns(Monster_pos_end(B));
    ns(Monster_name_create_str(B, "MyMonster"));
    ns(Monster_inventory_create(B, inv, c_vec_len(inv)));
    ns(Monster_test4_start(B));
    test = ns(Monster_test4_extend(B, 1));
    test->a = 0x10;
    test->b = 0x20;
    test = ns(Monster_test4_extend(B, 2));
    test->a = 0x30;
    test->b = 0x40;
    test[1].a = 0x50;
    test[1].b = 0x60;
    ns(Monster_test4_push_create(B, 0x70, (int8_t)0x80));
    x.a = 0x191; /* This is a short. */
    x.b = (int8_t)0x91; /* This is a byte. */
    ns(Monster_test4_push(B, &x));
    ns(Monster_test4_end(B));
    ns(Monster_end_as_root(B));

    return 0;
}

int time_monster(flatcc_builder_t *B)
{
    double t1, t2;
    const int rep = 1000000;
    size_t size;
    int i;

    printf("start timing ...\n");
    t1 = elapsed_realtime();
    for (i = 0; i < rep; ++i) {
        gen_monster_benchmark(B);
    }
    size = flatcc_builder_get_buffer_size(B);
    t2 = elapsed_realtime();
    show_benchmark("encode monster buffer", t1, t2, size, rep, "million");
    return 0;
}

int gen_struct_buffer_benchmark(flatcc_builder_t *B)
{
    void *buffer;
    ns(Vec3_t) *v;
    ns(Vec3_struct_t) vec3;

    flatcc_builder_reset(B);

    ns(Vec3_create_as_root(B, 1, 2, 3, 4.2, ns(Color_Blue), 2730, -17));

    buffer = flatcc_builder_get_direct_buffer(B, 0);
    if (!buffer) {
        return -1;
    }
    vec3 = ns(Vec3_as_root_with_identifier(buffer, 0));
    /* Convert buffer to native in place - a nop on native platform. */
    v = (ns(Vec3_t) *)vec3;
    ns(Vec3_from_pe(v));
    if (v->x != 1.0f || v->y != 2.0f || v->z != 3.0f
        || v->test1 != 4.2 || v->test2 != ns(Color_Blue)
        || v->test3.a != 2730 || v->test3.b != -17
       ) {
        return -1;
    }
    return 0;
}

int time_struct_buffer(flatcc_builder_t *B)
{
    double t1, t2;
    const int rep = 1000000;
    size_t size;
    int i;
    int ret = 0;

    printf("start timing ...\n");
    t1 = elapsed_realtime();
    for (i = 0; i < rep; ++i) {
        ret |= gen_struct_buffer_benchmark(B);
    }
    t2 = elapsed_realtime();
    size = flatcc_builder_get_buffer_size(B);
    if (ret) {
        printf("struct not valid\n");
    }
    show_benchmark("encode, decode and access Vec struct buffers", t1, t2, size, rep, "million");
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
    printf("running optimized monster test\n");
#else
    printf("running debug monster test\n");
#endif
#if 1
    if (test_enums(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_empty_monster(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_monster(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_monster_with_size(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_string(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_struct_buffer(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_typed_empty_monster(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_typed_struct_buffer(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_clone_slice(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_add_set_defaults(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_create_add_field(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_union_vector(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_basic_sort(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_sort_find(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_scan(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_nested_buffer(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_nested_buffer_first(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_nested_buffer_using_nest(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_cloned_monster(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (verify_include(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_type_aliases(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_mixed_type_union(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_recursive_sort(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif
#if 1
    if (test_fixed_length_array(B)) {
        printf("TEST FAILED\n");
        return -1;
    }
#endif

#ifdef FLATBUFFERS_BENCHMARK
    time_monster(B);
    time_struct_buffer(B);
#endif
    flatcc_builder_clear(B);
    return 0;
}
