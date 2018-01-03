#include <stdio.h>
#include "monster_test_json_parser.h"
#include "monster_test_json_printer.h"
#include "monster_test_verifier.h"

#include "flatcc/support/hexdump.h"

#define UQL FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST
#define UQ FLATCC_JSON_PARSE_ALLOW_UNQUOTED

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

int test_json(char *json, char *expect, int expect_err, int parse_flags, int print_flags, int line)
{
    int ret = -1;
    int err;
    void *flatbuffer = 0;
    char *buf = 0;
    size_t flatbuffer_size, buf_size;
    flatcc_builder_t builder, *B;
    flatcc_json_parser_t parser;
    flatcc_json_printer_t printer;
    int i;

    B = &builder;
    flatcc_builder_init(B);
    flatcc_json_printer_init_dynamic_buffer(&printer, 0);
    flatcc_json_printer_set_flags(&printer, print_flags);

    err = monster_test_parse_json(B, &parser, json, strlen(json), parse_flags);
    if (err != expect_err) {
        if (expect_err)
        {
            if( err )
            {
                fprintf(stderr, "%d: json test: parse failed with: %s\n", line, flatcc_json_parser_error_string(err));
                fprintf(stderr, "but expected to fail with: %s\n", flatcc_json_parser_error_string(expect_err));
                fprintf(stderr, "%s\n", json);
            }
            else
            {
                fprintf(stderr, "%d: json test: parse successful, but expected to fail with: %s\n", line, flatcc_json_parser_error_string(expect_err));
                fprintf(stderr, "%s\n", json);
            }
        }
        else
        {
            fprintf(stderr, "%d: json test: parse failed: %s\n", line, flatcc_json_parser_error_string(err));
            fprintf(stderr, "%s\n", json);
        }
        for (i = 0; i < parser.pos - 1; ++i) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n");
        goto failed;
    }
    if (expect_err) {
        ret = 0;
        goto done;
    }
    flatbuffer = flatcc_builder_finalize_aligned_buffer(B, &flatbuffer_size);
    if ((ret = ns(Monster_verify_as_root(flatbuffer, flatbuffer_size)))) {
        fprintf(stderr, "%s:%d: buffer verification failed: %s\n", __FILE__, line, flatcc_verify_error_string(ret));
        goto failed;
    }

    ns(Monster_print_json_as_root(&printer, flatbuffer, flatbuffer_size, "MONS"));
    buf = flatcc_json_printer_get_buffer(&printer, &buf_size);
    if (!buf || strcmp(expect, buf)) {
        fprintf(stderr, "%d: json test: printed buffer not as expected, got:\n", line);
        fprintf(stderr, "%s\n", buf);
        fprintf(stderr, "expected:\n");
        fprintf(stderr, "%s\n", expect);
        goto failed;
    }

    ret = 0;
done:
    if (flatbuffer) {
        aligned_free(flatbuffer);
    }
    flatcc_builder_clear(B);
    flatcc_json_printer_clear(&printer);
    return ret;
failed:
    if (flatbuffer) {
        hexdump("parsed buffer", flatbuffer, flatbuffer_size, stderr);
    }

    ret = -1;
    goto done;
}

#define TEST(x, y) \
    ret |= test_json((x), (y), 0, 0, 0, __LINE__);

#define TEST_ERROR(x, err) \
    ret |= test_json((x), 0, err, 0, 0, __LINE__);

#define TEST_FLAGS(fparse, fprint, x, y) \
    ret |= test_json((x), (y), 0, (fparse), (fprint), __LINE__);

int edge_case_tests()
{
    int ret = 0;
/*
 * Each symbolic value is type coerced and added. One might expect
 * or'ing flags together, but it doesn't work with signed values
 * and floating point target values. We would either need a much
 * more complicated parser or restrict the places where symbols are
 * allowed.
 */
#if 0
    TEST(   "{ name: \"Monster\", color: \"Green Blue Red Blue\"}",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");
#else
#if UQ
    TEST(   "{ name: \"Monster\", color: \"Green Blue Red Blue\"}",
            "{\"name\":\"Monster\",\"color\":19}");
#else
    TEST(   "{ \"name\": \"Monster\", \"color\": \"Green Blue Red Blue\"}",
            "{\"name\":\"Monster\",\"color\":19}");
#endif
#endif

/*
 * If a value is stored, even if default, it is also printed.
 * This option can also be flagged compile time for better performance.
 */
    TEST_FLAGS(flatcc_json_parser_f_force_add, 0,
            "{ \"name\": \"Monster\", \"color\": 8}",
            "{\"name\":\"Monster\",\"color\":\"Blue\"}");

    TEST_FLAGS(0, flatcc_json_printer_f_noenum,
            "{ \"name\": \"Monster\", \"color\": \"Green\"}",
            "{\"name\":\"Monster\",\"color\":2}");

    TEST_FLAGS(flatcc_json_parser_f_force_add, flatcc_json_printer_f_skip_default,
            "{ \"name\": \"Monster\", \"color\": 8}",
            "{\"name\":\"Monster\"}");

    TEST_FLAGS(0, flatcc_json_printer_f_force_default,
            "{ \"name\": \"Monster\"}",
"{\"mana\":150,\"hp\":100,\"name\":\"Monster\",\"color\":\"Blue\",\"testbool\":true,\"testhashs32_fnv1\":0,\"testhashu32_fnv1\":0,\"testhashs64_fnv1\":0,\"testhashu64_fnv1\":0,\"testhashs32_fnv1a\":0,\"testhashu32_fnv1a\":0,\"testhashs64_fnv1a\":0,\"testhashu64_fnv1a\":0}");

    TEST_FLAGS(flatcc_json_parser_f_force_add, 0,
            "{ \"name\": \"Monster\", \"color\": \"Blue\"}",
            "{\"name\":\"Monster\",\"color\":\"Blue\"}");

    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, 0,
            "{ \"name\": \"Monster\", \"xcolor\": \"Green\", \"hp\": 42}",
            "{\"hp\":42,\"name\":\"Monster\"}");

    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, flatcc_json_printer_f_unquote,
            "{ \"name\": \"Monster\", \"xcolor\": \"Green\", \"hp\": 42}",
            "{hp:42,name:\"Monster\"}");

    /* Also test generic parser used with unions with late type field. */
    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, 0,
            "{ \"name\": \"Monster\", \"xcolor\": \"Green\", "
            "\"foobar\": { \"a\": [1, 2.0, ], \"a1\": {}, \"b\": null, \"c\":[],  }, \"hp\": 42 }",
            "{\"hp\":42,\"name\":\"Monster\"}");
#if UQ
/*
 * If a value is stored, even if default, it is also printed.
 * This option can also be flagged compile time for better performance.
 */
    TEST_FLAGS(flatcc_json_parser_f_force_add, 0,
            "{ name: \"Monster\", color: 8}",
            "{\"name\":\"Monster\",\"color\":\"Blue\"}");

    TEST_FLAGS(0, flatcc_json_printer_f_noenum,
            "{ name: \"Monster\", color: Green}",
            "{\"name\":\"Monster\",\"color\":2}");

    TEST_FLAGS(flatcc_json_parser_f_force_add, flatcc_json_printer_f_skip_default,
            "{ name: \"Monster\", color: 8}",
            "{\"name\":\"Monster\"}");

    TEST_FLAGS(0, flatcc_json_printer_f_force_default,
            "{ name: \"Monster\"}",
"{\"mana\":150,\"hp\":100,\"name\":\"Monster\",\"color\":\"Blue\",\"testbool\":true,\"testhashs32_fnv1\":0,\"testhashu32_fnv1\":0,\"testhashs64_fnv1\":0,\"testhashu64_fnv1\":0,\"testhashs32_fnv1a\":0,\"testhashu32_fnv1a\":0,\"testhashs64_fnv1a\":0,\"testhashu64_fnv1a\":0}");

    TEST_FLAGS(flatcc_json_parser_f_force_add, 0,
            "{ name: \"Monster\", color: Blue}",
            "{\"name\":\"Monster\",\"color\":\"Blue\"}");

    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, 0,
            "{ name: \"Monster\", xcolor: Green, hp: 42}",
            "{\"hp\":42,\"name\":\"Monster\"}");

    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, flatcc_json_printer_f_unquote,
            "{ name: \"Monster\", xcolor: Green, hp: 42}",
            "{hp:42,name:\"Monster\"}");

    /* Also test generic parser used with unions with late type field. */
    TEST_FLAGS(flatcc_json_parser_f_skip_unknown, 0,
            "{ name: \"Monster\", xcolor: Green, "
            "foobar: { a: [1, 2.0, ], a1: {}, b: null, c:[],  }, hp: 42 }",
            "{\"hp\":42,\"name\":\"Monster\"}");
#endif

/* Without skip unknown, we should expect failure. */
#if 0
    TEST(   "{ name: \"Monster\", xcolor: Green}",
            "{\"name\":\"Monster\"}");
#endif

/* We do not support null. */
#if 0
    TEST(
            "{ name: \"Monster\", test_type: null }",
            "{\"name\":\"Monster\"}");
#endif

/*
 * We do not allow empty flag strings because they might mean
 * either default value, or 0.
 */
#if 0
    /* Questionable if this really is an error. */
    TEST(   "{ name: \"Monster\", color: \"\"}",
            "{\"name\":\"Monster\",\"color\":0}"); // TODO: should this be color:"" ?

    TEST(   "{ name: \"Monster\", color: \" \"}",
            "{\"name\":\"Monster\",\"color\":0}");

#endif

    return ret;
}

int error_case_tests()
{
    int ret = 0;

    TEST_ERROR( "{ \"nickname\": \"Monster\" }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"test_type\": \"Monster\", \"test\": { \"nickname\": \"Joker\", \"color\": \"Red\" } } }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"test_type\": \"Monster\", \"test\": { \"name\": \"Joker\", \"colour\": \"Red\" } } }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": [ { \"nickname\": \"Joker\", \"color\": \"Red\" } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": [ { \"name\": \"Joker\", \"colour\": \"Red\" } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": ["
                "{ \"name\": \"Joker\", \"color\": \"Red\", \"test_type\": \"Monster\", \"test\": { \"nickname\": \"Harley\", \"color\": \"Blue\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": ["
                "{ \"name\": \"Joker\", \"color\": \"Red\", \"test_type\": \"Monster\", \"test\": { \"name\": \"Harley\", \"colour\": \"Blue\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": ["
                "{ \"name\": \"Joker\", \"test_type\": \"Monster\", \"test\": { \"nickname\": \"Harley\" } },"
                "{ \"name\": \"Bonnie\", \"test_type\": \"Monster\", \"test\": { \"name\": \"Clyde\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ \"name\": \"Monster\", \"testarrayoftables\": ["
                "{ \"name\": \"Joker\", \"test_type\": \"Monster\", \"test\": { \"name\": \"Harley\" } },"
                "{ \"name\": \"Bonnie\", \"test_type\": \"Monster\", \"test\": { \"nickname\": \"Clyde\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );

#if !UQ
    TEST_ERROR( "{ nickname: \"Monster\" }",
                flatcc_json_parser_error_unexpected_character );

    TEST_ERROR( "{ \"name\": \"Monster\", \"color\": Green }",
                flatcc_json_parser_error_unexpected_character );

    TEST_ERROR( "{ \"name\": \"Monster\", \"color\":    Green  Red   Blue    }",
                flatcc_json_parser_error_unexpected_character );
#endif

#if UQ
    TEST_ERROR( "{ nickname: \"Monster\" }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", test_type: Monster, test: { nickname: \"Joker\", color: \"Red\" } } }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", test_type: Monster, test: { name: \"Joker\", colour: \"Red\" } } }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: [ { nickname: \"Joker\", color: \"Red\" } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: [ { name: \"Joker\", colour: \"Red\" } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: ["
                "{ name: \"Joker\", color: \"Red\", test_type: Monster, test: { nickname: \"Harley\", color: \"Blue\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: ["
                "{ name: \"Joker\", color: \"Red\", test_type: Monster, test: { name: \"Harley\", colour: \"Blue\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: ["
                "{ name: \"Joker\", test_type: Monster, test: { nickname: \"Harley\" } },"
                "{ name: \"Bonnie\", test_type: Monster, test: { name: \"Clyde\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );
    TEST_ERROR( "{ name: \"Monster\", testarrayoftables: ["
                "{ name: \"Joker\", test_type: Monster, test: { name: \"Harley\" } },"
                "{ name: \"Bonnie\", test_type: Monster, test: { nickname: \"Clyde\" } } ] }",
                flatcc_json_parser_error_unknown_symbol );

#endif

    return ret;
}

#define RANDOM_BASE64 "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY/5pKYmk+54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG+64Zv+Uygw=="

#define RANDOM_BASE64_NOPAD "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY/5pKYmk+54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG+64Zv+Uygw"

#define RANDOM_BASE64URL "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY_5pKYmk-54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG-64Zv-Uygw=="

#define RANDOM_BASE64URL_NOPAD "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY_5pKYmk-54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG-64Zv-Uygw"

int base64_tests()
{
    int ret = 0;

    /* Reference */
    TEST(   "{ \"name\": \"Monster\" }",
            "{\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{} }",
            "{\"name\":\"Monster\",\"testbase64\":{}}");


    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{ \"data\":\"" RANDOM_BASE64 "\"} }",
            "{\"name\":\"Monster\",\"testbase64\":{\"data\":\"" RANDOM_BASE64 "\"}}");

    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{ \"urldata\":\"" RANDOM_BASE64URL "\"} }",
            "{\"name\":\"Monster\",\"testbase64\":{\"urldata\":\"" RANDOM_BASE64URL "\"}}");

    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{ \"data\":\"" RANDOM_BASE64_NOPAD "\"} }",
            "{\"name\":\"Monster\",\"testbase64\":{\"data\":\"" RANDOM_BASE64 "\"}}");

    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{ \"urldata\":\"" RANDOM_BASE64URL_NOPAD "\"} }",
            "{\"name\":\"Monster\",\"testbase64\":{\"urldata\":\"" RANDOM_BASE64URL "\"}}");

    TEST_ERROR(   "{ \"name\": \"Monster\", \"testbase64\":{ \"data\":\"" RANDOM_BASE64URL "\"} }",
            flatcc_json_parser_error_base64);

    TEST_ERROR(   "{ \"name\": \"Monster\", \"testbase64\":{ \"urldata\":\"" RANDOM_BASE64 "\"} }",
            flatcc_json_parser_error_base64url);

#if 0
    /*
     * Our test framework is not geared towards properly testing nested
     * flatbuffers in base64 JSON, but if the following yields a verification error,
     * the nested field has parsed a base64 buffer with garbage.
     */
    TEST(   "{ \"name\": \"Monster\", \"testbase64\":{ \"nested\":\"" RANDOM_BASE64 "\"} }",
            "{\"name\":\"Monster\",\"testbase64\":{\"nested\":\"" RANDOM_BASE64 "\"}}");
#endif

    return ret;
}

int mixed_type_union_tests()
{
    int ret = 0;

    /* Reference */
    TEST(   "{ \"name\": \"Monster\" }",
            "{\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":{} }",
            "{\"name\":\"Monster\",\"movie\":{}}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 } }}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19}}}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Other\", \"side_kick\": \"a donkey\"}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\"}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42}}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}}");

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42},"
            " \"characters_type\": [], \"characters\": []}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\","
            "\"characters_type\":[],\"characters\":[]}}")

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42},"
            " \"characters_type\": [\"Fantasy.Character.Rapunzel\", \"Other\", 0, \"MuLan\"],"
            " \"characters\": [{\"hair_length\":19}, \"unattributed extras\", null, {\"sword_attack_damage\":2}]}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\","
            "\"characters_type\":[\"Rapunzel\",\"Other\",\"NONE\",\"MuLan\"],"
            "\"characters\":[{\"hair_length\":19},\"unattributed extras\",null,{\"sword_attack_damage\":2}]}}")

    TEST(   "{ \"name\": \"Monster\", \"movie\":"
            "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Character.Other\", \"side_kick\": \"a donkey\"}}",
            "{\"name\":\"Monster\",\"movie\":"
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}}");

    return ret;
}

/*
 * Here we cover some border cases around unions and flag
 * enumerations, and nested buffers.
 *
 * More complex objects with struct members etc. are reasonably
 * covered in the printer and parser tests using the golden data
 * set.
 */
int main()
{
    int ret = 0;

    ret |= edge_case_tests();
    ret |= error_case_tests();
    ret |= base64_tests();
    ret |= mixed_type_union_tests();

    /* Allow trailing comma. */
    TEST(   "{ \"name\": \"Monster\", }",
            "{\"name\":\"Monster\"}");

    TEST(   "{\"color\": \"Red\",  \"name\": \"Monster\", }",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"Green\" }",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"Green Red Blue\" }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"   Green  Red   Blue   \" }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"Red\" }",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\"  :\"Green\" }",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    /* Default value. */
    TEST(   "{ \"name\": \"Monster\", \"color\": \"Blue\" }",
            "{\"name\":\"Monster\"}");

    /* Default value. */
    TEST(   "{ \"name\": \"Monster\", \"color\": 8}",
            "{\"name\":\"Monster\"}");
#if UQ
    /* Allow trailing comma. */
    TEST(   "{ name: \"Monster\", }",
            "{\"name\":\"Monster\"}");

    TEST(   "{color: \"Red\",  name: \"Monster\", }",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ name: \"Monster\", color: \"Green\" }",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    TEST(   "{ name: \"Monster\", color: \"Green Red Blue\" }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ name: \"Monster\", color: \"   Green  Red   Blue   \" }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ name: \"Monster\", color: Red }",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ name: \"Monster\", color: Green }",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    /* Default value. */
    TEST(   "{ name: \"Monster\", color: Blue }",
            "{\"name\":\"Monster\"}");

    /* Default value. */
    TEST(   "{ name: \"Monster\", color: 8}",
            "{\"name\":\"Monster\"}");
#endif
#if UQL
    TEST(   "{ name: \"Monster\", color: Green Red }",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");
#endif

#if UQL
    /* No leading space in unquoted flag. */
    TEST(   "{ name: \"Monster\", color:Green Red }",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");

    TEST(   "{ name: \"Monster\", color: Green Red}",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");

    TEST(   "{ name: \"Monster\", color:Green   Blue Red   }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");
#endif

    TEST(   "{ \"name\": \"Monster\", \"color\": 1}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 2}",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 9}",
            "{\"name\":\"Monster\",\"color\":\"Red Blue\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 11}",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 12}",
            "{\"name\":\"Monster\",\"color\":12}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 15}",
            "{\"name\":\"Monster\",\"color\":15}");

    TEST(   "{ \"name\": \"Monster\", \"color\": 0}",
            "{\"name\":\"Monster\",\"color\":0}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"Color.Red\"}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ \"name\": \"Monster\", \"color\": \"MyGame.Example.Color.Red\"}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ \"name\": \"Monster\", \"hp\": \"Color.Green\"}",
            "{\"hp\":2,\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"hp\": \"Color.Green\"}",
            "{\"hp\":2,\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\": \"Monster\", \"test\": { \"name\": \"any Monster\" } }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"any Monster\"}}");

    /* This is tricky because the test field must be reparsed after discovering the test type. */
    TEST(   "{ \"name\": \"Monster\", \"test\": { \"name\": \"second Monster\" }, \"test_type\": \"Monster\"  }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\"}}");

    /* Also test that parsing can continue after reparse. */
    TEST(   "{ \"name\": \"Monster\", \"test\": { \"name\": \"second Monster\" }, \"hp\":17, \"test_type\":\n \"Monster\", \"color\":\"Green\" }",
            "{\"hp\":17,\"name\":\"Monster\",\"color\":\"Green\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\"}}");

    /* Test that NONE is recognized, and that we do not get a missing table error.*/
    TEST(   "{ \"name\": \"Monster\", \"test_type\": \"NONE\" }",
            "{\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\": 0 }",
            "{\"name\":\"Monster\"}");

#if UQ
    TEST(   "{ name: \"Monster\", color: 1}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ name: \"Monster\", color: 2}",
            "{\"name\":\"Monster\",\"color\":\"Green\"}");

    TEST(   "{ name: \"Monster\", color: 9}",
            "{\"name\":\"Monster\",\"color\":\"Red Blue\"}");

    TEST(   "{ name: \"Monster\", color: 11}",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");

    TEST(   "{ name: \"Monster\", color: 12}",
            "{\"name\":\"Monster\",\"color\":12}");

    TEST(   "{ name: \"Monster\", color: 15}",
            "{\"name\":\"Monster\",\"color\":15}");

    TEST(   "{ name: \"Monster\", color: 0}",
            "{\"name\":\"Monster\",\"color\":0}");

    TEST(   "{ name: \"Monster\", color: Color.Red}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ name: \"Monster\", color: MyGame.Example.Color.Red}",
            "{\"name\":\"Monster\",\"color\":\"Red\"}");

    TEST(   "{ name: \"Monster\", hp: Color.Green}",
            "{\"hp\":2,\"name\":\"Monster\"}");

    TEST(   "{ name: \"Monster\", hp: Color.Green}",
            "{\"hp\":2,\"name\":\"Monster\"}");

    TEST(   "{ name: \"Monster\", test_type: Monster, test: { name: \"any Monster\" } }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"any Monster\"}}");

    /* This is tricky because the test field must be reparsed after discovering the test type. */
    TEST(   "{ name: \"Monster\", test: { name: \"second Monster\" }, test_type: Monster  }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\"}}");

    /* Also test that parsing can continue after reparse. */
    TEST(   "{ name: \"Monster\", test: { name: \"second Monster\" }, hp:17, test_type:\n Monster, color:Green }",
            "{\"hp\":17,\"name\":\"Monster\",\"color\":\"Green\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\"}}");

    /* Test that NONE is recognized, and that we do not get a missing table error.*/
    TEST(   "{ name: \"Monster\", test_type: NONE }",
            "{\"name\":\"Monster\"}");

    TEST(   "{ name: \"Monster\", test_type: 0 }",
            "{\"name\":\"Monster\"}");

#endif

#if UQL
    /*
     * Test that generic parsing handles multiple flags correctly during
     * first pass before backtracking.
     */
    TEST(   "{ name: \"Monster\", test: { name: \"second Monster\", color: Red Green }, test_type: Monster  }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\",\"color\":\"Red Green\"}}");
#endif

    /* Ditto quoted flags. */
    TEST(   "{ \"name\": \"Monster\", \"test\": { \"name\": \"second Monster\", \"color\": \" Red Green \" }, \"test_type\": \"Monster\"  }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\",\"color\":\"Red Green\"}}");

    /*
     * Note the '\/' becomes just '/', and that '/' also works in input.
     *
     * The json printer does not have a concept of \x it always uses
     * unicode.
     *
     * We use json extension \x to inject a control 03 which extends
     * to a printed unicode escape, while the \u00F8 is a valid
     * character after encoding, and is thus not escaped after printing
     * but rather becoems a two-byte utf-8 encoding of 'ø' which
     * we use C encoding to form utf8 C3B8 == \u00F8.
     */
    TEST(   "{ \"name\": \"Mon\xfF\xFf\\x03s\\xC3\\xF9\\u00F8ter\\b\\f\\n\\r\\t\\\"\\\\\\/'/\", }",
            "{\"name\":\"Mon\xff\xff\\u0003s\xc3\xf9\xc3\xb8ter\\b\\f\\n\\r\\t\\\"\\\\/'/\"}");

    TEST(   "{ \"name\": \"\\u168B\\u1691\"}",
            "{\"name\":\"\xe1\x9a\x8b\xe1\x9a\x91\"}");

    /* Nested flatbuffer, either is a known object, or as a vector. */
    TEST(   "{ \"name\": \"Monster\", \"testnestedflatbuffer\":{ \"name\": \"sub Monster\" } }",
            "{\"name\":\"Monster\",\"testnestedflatbuffer\":{\"name\":\"sub Monster\"}}");

#if FLATBUFFERS_PROTOCOL_IS_LE
    TEST(   "{ \"name\": \"Monster\", \"testnestedflatbuffer\":"
            "[" /* start of nested flatbuffer, implicit size: 40 */
            "4,0,0,0," /* header: object offset = 4, no identifier */
            "248,255,255,255," /* vtable offset */
            "16,0,0,0," /* offset to name */
            "12,0,8,0,0,0,0,0,0,0,4,0," /* vtable */
            "11,0,0,0,115,117,98,32,77,111,110,115,116,101,114,0" /* name = "sub Monster" */
            "]" /* end of nested flatbuffer */
            "}",
            "{\"name\":\"Monster\",\"testnestedflatbuffer\":{\"name\":\"sub Monster\"}}");
#else
    TEST(   "{ \"name\": \"Monster\", \"testnestedflatbuffer\":"
            "[" /* start of nested flatbuffer, implicit size: 40 */
            "0,0,0,4," /* header: object offset = 4, no identifier */
            "255,255,255,248," /* vtable offset */
            "0,0,0,16," /* offset to name */
            "0,12,0,8,0,0,0,0,0,0,0,4," /* vtable */
            "0,0,0,11,115,117,98,32,77,111,110,115,116,101,114,0" /* name = "sub Monster" */
            "]" /* end of nested flatbuffer */
            "}",
            "{\"name\":\"Monster\",\"testnestedflatbuffer\":{\"name\":\"sub Monster\"}}");
#endif

    /* Test empty table */
    TEST(   "{ \"name\": \"Monster\", \"testempty\": {} }",
            "{\"name\":\"Monster\",\"testempty\":{}}" );

    /* Test empty array */
    TEST(   "{ \"name\": \"Monster\", \"testarrayoftables\": [] }",
            "{\"name\":\"Monster\",\"testarrayoftables\":[]}" );

    /* Test JSON prefix parsing */
    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing\": { \"aaaa\": \"test\", \"aaaa12345\": 17 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing\":{\"aaaa\":\"test\",\"aaaa12345\":17}}" );

    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing\": { \"bbbb\": \"test\", \"bbbb1234\": 19 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing\":{\"bbbb\":\"test\",\"bbbb1234\":19}}" );

    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing\": { \"cccc\": \"test\", \"cccc1234\": 19, \"cccc12345\": 17 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing\":{\"cccc\":\"test\",\"cccc1234\":19,\"cccc12345\":17}}" );

    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing\": { \"dddd1234\": 19, \"dddd12345\": 17 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing\":{\"dddd1234\":19,\"dddd12345\":17}}" );

    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing2\": { \"aaaa_bbbb_steps\": 19, \"aaaa_bbbb_start_\": 17 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing2\":{\"aaaa_bbbb_steps\":19,\"aaaa_bbbb_start_\":17}}" );

    TEST(   "{ \"name\": \"Monster\", \"testjsonprefixparsing3\": { \"aaaa_bbbb_steps\": 19, \"aaaa_bbbb_start_steps\": 17 } }",
            "{\"name\":\"Monster\",\"testjsonprefixparsing3\":{\"aaaa_bbbb_steps\":19,\"aaaa_bbbb_start_steps\":17}}" );

    /* Union vector */

    TEST(   "{ \"name\": \"Monster\", \"manyany_type\": [ \"Monster\" ], \"manyany\": [{\"name\": \"Joe\"}] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\"}]}");

    TEST(   "{ \"name\": \"Monster\", \"manyany_type\": [ \"NONE\" ], \"manyany\": [ null ] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"NONE\"],\"manyany\":[null]}");

    TEST(   "{ \"name\": \"Monster\", \"manyany_type\": [ \"Monster\", \"NONE\" ], \"manyany\": [{\"name\": \"Joe\"}, null] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"Monster\",\"NONE\"],\"manyany\":[{\"name\":\"Joe\"},null]}");

    TEST(   "{ \"name\": \"Monster\", \"manyany_type\": [ \"Monster\" ], \"manyany\": [ { \"name\":\"Joe\", \"test_type\": \"Monster\", \"test\": { \"name\": \"any Monster\" } } ] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\",\"test_type\":\"Monster\",\"test\":{\"name\":\"any Monster\"}}]}");

    TEST(   "{ \"name\": \"Monster\", \"manyany\": [{\"name\": \"Joe\"}], \"manyany_type\": [ \"Monster\" ] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\"}]}");

    TEST(   "{ \"name\": \"Monster\", \"manyany\": [{\"name\": \"Joe\", \"manyany\":[null, null], \"manyany_type\": [\"NONE\", \"NONE\"]}], \"manyany_type\": [ \"Monster\" ] }",
            "{\"name\":\"Monster\",\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\",\"manyany_type\":[\"NONE\",\"NONE\"],\"manyany\":[null,null]}]}");

    return ret ? -1: 0;
}
