#include <stdio.h>
#include "monster_test_json_parser.h"
#include "monster_test_json_printer.h"
#include "monster_test_verifier.h"

#include "flatcc/support/hexdump.h"

#define UQL FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST
#define UQ FLATCC_JSON_PARSE_ALLOW_UNQUOTED

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

#undef nsf
#define nsf(x) FLATBUFFERS_WRAP_NAMESPACE(Fantasy, x)

struct test_scope {
    const char *identifier;
    flatcc_json_parser_table_f *parser;
    flatcc_json_printer_table_f *printer;
    flatcc_table_verifier_f *verifier;
};

static const struct test_scope Monster = {
    /* The is the schema global file identifier. */
    ns(Monster_file_identifier),
    ns(Monster_parse_json_table),
    ns(Monster_print_json_table),
    ns(Monster_verify_table)
};

static const struct test_scope Alt = {
    /* This is the type hash identifier. */
    ns(Alt_type_identifier),
    ns(Alt_parse_json_table),
    ns(Alt_print_json_table),
    ns(Alt_verify_table)
};

static const struct test_scope Movie = {
    /* This is the type hash identifier. */
    nsf(Movie_type_identifier),
    nsf(Movie_parse_json_table),
    nsf(Movie_print_json_table),
    nsf(Movie_verify_table)
};

int test_json(const struct test_scope *scope, char *json,
        char *expect, int expect_err,
        flatcc_json_parser_flags_t parse_flags, flatcc_json_printer_flags_t print_flags, int line)
{
    int ret = -1;
    int err;
    void *flatbuffer = 0;
    char *buf = 0;
    size_t flatbuffer_size, buf_size;
    flatcc_builder_t builder, *B;
    flatcc_json_parser_t parser_ctx;
    flatcc_json_printer_t printer_ctx;
    int i;

    B = &builder;
    flatcc_builder_init(B);
    flatcc_json_printer_init_dynamic_buffer(&printer_ctx, 0);
    flatcc_json_printer_set_flags(&printer_ctx, print_flags);
    err = flatcc_json_parser_table_as_root(B, &parser_ctx, json, strlen(json), parse_flags,
            scope->identifier, scope->parser);
    if (err != expect_err) {
        if (expect_err) {
            if (err) {
                fprintf(stderr, "%d: json test: parse failed with: %s\n",
                        line, flatcc_json_parser_error_string(err));
                fprintf(stderr, "but expected to fail with: %s\n",
                        flatcc_json_parser_error_string(expect_err));
                fprintf(stderr, "%s\n", json);
            } else {
                fprintf(stderr, "%d: json test: parse successful, but expected to fail with: %s\n",
                        line, flatcc_json_parser_error_string(expect_err));
                fprintf(stderr, "%s\n", json);
            }
        } else {
            fprintf(stderr, "%d: json test: parse failed: %s\n", line, flatcc_json_parser_error_string(err));
            fprintf(stderr, "%s\n", json);
        }
        for (i = 0; i < parser_ctx.pos - 1; ++i) {
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
    if ((ret = flatcc_verify_table_as_root(flatbuffer, flatbuffer_size, scope->identifier, scope->verifier))) {
        fprintf(stderr, "%s:%d: buffer verification failed: %s\n",
                __FILE__, line, flatcc_verify_error_string(ret));
        goto failed;
    }

    flatcc_json_printer_table_as_root(&printer_ctx, flatbuffer, flatbuffer_size, scope->identifier, scope->printer);
    buf = flatcc_json_printer_get_buffer(&printer_ctx, &buf_size);
    if (!buf || strcmp(expect, buf)) {
        fprintf(stderr, "%d: json test: printed buffer not as expected, got:\n", line);
        fprintf(stderr, "%s\n", buf);
        fprintf(stderr, "expected:\n");
        fprintf(stderr, "%s\n", expect);
        goto failed;
    }
    ret = 0;

done:
    flatcc_builder_aligned_free(flatbuffer);
    flatcc_builder_clear(B);
    flatcc_json_printer_clear(&printer_ctx);
    return ret;

failed:
    if (flatbuffer) {
        hexdump("parsed buffer", flatbuffer, flatbuffer_size, stderr);
    }
    ret = -1;
    goto done;
}

#define BEGIN_TEST(name) int ret = 0; const struct test_scope *scope = &name
#define END_TEST() return ret;

#define TEST(x, y) \
    ret |= test_json(scope, (x), (y), 0, 0, 0, __LINE__);

#define TEST_ERROR(x, err) \
    ret |= test_json(scope, (x), 0, err, 0, 0, __LINE__);

#define TEST_FLAGS(fparse, fprint, x, y) \
    ret |= test_json(scope, (x), (y), 0, (fparse), (fprint), __LINE__);

#define TEST_ERROR_FLAGS(fparse, fprint, x, err) \
    ret |= test_json(scope, (x), 0, err, (fparse), (fprint), __LINE__);

int edge_case_tests(void)
{
    BEGIN_TEST(Monster);
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
            "{ \"name\": \"Monster\", \"testf\":3.0}",
"{\"mana\":150,\"hp\":100,\"name\":\"Monster\",\"color\":\"Blue\",\"testbool\":true,\"testhashs32_fnv1\":0,\"testhashu32_fnv1\":0,\"testhashs64_fnv1\":0,\"testhashu64_fnv1\":0,\"testhashs32_fnv1a\":0,\"testhashu32_fnv1a\":0,\"testhashs64_fnv1a\":0,\"testhashu64_fnv1a\":0,\"testf\":3,\"testf2\":3,\"testf3\":0}");


    /* 
     * Cannot test the default of testf field because float is printed as double with
     * configuration dependent precision.
     */
#if 0 
    TEST_FLAGS(0, flatcc_json_printer_f_force_default,
            "{ \"name\": \"Monster\", \"testf3\":3.14159012}",
"{\"mana\":150,\"hp\":100,\"name\":\"Monster\",\"color\":\"Blue\",\"testbool\":true,\"testhashs32_fnv1\":0,\"testhashu32_fnv1\":0,\"testhashs64_fnv1\":0,\"testhashu64_fnv1\":0,\"testhashs32_fnv1a\":0,\"testhashu32_fnv1a\":0,\"testhashs64_fnv1a\":0,\"testhashu64_fnv1a\":0,\"testf\":3.14159,\"testf2\":3,\"testf3\":0}");
#endif

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
"{\"mana\":150,\"hp\":100,\"name\":\"Monster\",\"color\":\"Blue\",\"testbool\":true,\"testhashs32_fnv1\":0,\"testhashu32_fnv1\":0,\"testhashs64_fnv1\":0,\"testhashu64_fnv1\":0,\"testhashs32_fnv1a\":0,\"testhashu32_fnv1a\":0,\"testhashs64_fnv1a\":0,\"testhashu64_fnv1a\":0,\"testf\":314159,\"testf2\":3,\"testf3\":0}");

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

    END_TEST();
}

int error_case_tests(void)
{
    BEGIN_TEST(Monster);

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

    END_TEST();
}

#define RANDOM_BASE64 "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY/5pKYmk+54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG+64Zv+Uygw=="

#define RANDOM_BASE64_NOPAD "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY/5pKYmk+54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG+64Zv+Uygw"

#define RANDOM_BASE64URL "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY_5pKYmk-54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG-64Zv-Uygw=="

#define RANDOM_BASE64URL_NOPAD "zLOuiUjH49tz4Ap2JnmpTX5NqoiMzlD8hSw45QCS2yaSp7UYoA" \
    "oE8KpY_5pKYmk-54NI40hyeyZ1zRUE4vKQT0hEdVl0iXq2fqPamkVD1AZlVvQJ1m00PaoXOSgG-64Zv-Uygw"

int base64_tests(void)
{
    BEGIN_TEST(Monster);

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

/* Test case from Googles flatc implementation. */
#if UQ
    TEST(    "{name: \"Monster\","
            "testbase64: {"
            "data: \"23A/47d450+sdfx9+wRYIS09ckas/asdFBQ=\","
            "urldata: \"23A_47d450-sdfx9-wRYIS09ckas_asdFBQ=\","
            "nested: \"FAAAAE1PTlMMAAwAAAAEAAYACAAMAAAAAAAAAAQAAAANAAAATmVzdGVkTW9uc3RlcgAAAA==\""
            "}}",
            "{\"name\":\"Monster\","
            "\"testbase64\":{"
            "\"data\":\"23A/47d450+sdfx9+wRYIS09ckas/asdFBQ=\","
            "\"urldata\":\"23A_47d450-sdfx9-wRYIS09ckas_asdFBQ=\","
            "\"nested\":\"FAAAAE1PTlMMAAwAAAAEAAYACAAMAAAAAAAAAAQAAAANAAAATmVzdGVkTW9uc3RlcgAAAA==\""
            "}}");

    TEST(   "{name: \"Monster\","
            "testbase64: {"
            "data: \"23A/47d450+sdfx9+wRYIS09ckas/asdFBQ\","
            "urldata: \"23A_47d450-sdfx9-wRYIS09ckas_asdFBQ\","
            "nested: \"FAAAAE1PTlMMAAwAAAAEAAYACAAMAAAAAAAAAAQAAAANAAAATmVzdGVkTW9uc3RlcgAAAA\""
            "}}",
            "{\"name\":\"Monster\","
            "\"testbase64\":{"
            "\"data\":\"23A/47d450+sdfx9+wRYIS09ckas/asdFBQ=\","
            "\"urldata\":\"23A_47d450-sdfx9-wRYIS09ckas_asdFBQ=\","
            "\"nested\":\"FAAAAE1PTlMMAAwAAAAEAAYACAAMAAAAAAAAAAQAAAANAAAATmVzdGVkTW9uc3RlcgAAAA==\""
            "}}");
#endif

    END_TEST();
}

int mixed_type_union_tests(void)
{
    BEGIN_TEST(Movie);

    /* Reference */

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 } }",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19}}");

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Other\", \"side_kick\": \"a donkey\"}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}");

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\"}}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}");

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42}}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}");

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42},"
            " \"characters_type\": [], \"characters\": []}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\","
            "\"characters_type\":[],\"characters\":[]}")

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Fantasy.Character.Other\", \"side_kick\": \"a donkey\","
            "  \"antagonist_type\": \"MuLan\", \"antagonist\": {\"sword_attack_damage\": 42},"
            " \"characters_type\": [\"Fantasy.Character.Rapunzel\", \"Other\", 0, \"MuLan\"],"
            " \"characters\": [{\"hair_length\":19}, \"unattributed extras\", null, {\"sword_attack_damage\":2}]}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"antagonist_type\":\"MuLan\",\"antagonist\":{\"sword_attack_damage\":42},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\","
            "\"characters_type\":[\"Rapunzel\",\"Other\",\"NONE\",\"MuLan\"],"
            "\"characters\":[{\"hair_length\":19},\"unattributed extras\",null,{\"sword_attack_damage\":2}]}")

    TEST(   "{ \"main_character_type\": \"Rapunzel\", \"main_character\": { \"hair_length\": 19 },"
            "  \"side_kick_type\": \"Character.Other\", \"side_kick\": \"a donkey\"}",
            "{\"main_character_type\":\"Rapunzel\",\"main_character\":{\"hair_length\":19},"
            "\"side_kick_type\":\"Other\",\"side_kick\":\"a donkey\"}");

    END_TEST();
}

int union_vector_tests(void)
{
    BEGIN_TEST(Alt);
    /* Union vector */

    TEST(   "{ \"manyany_type\": [ \"Monster\" ], \"manyany\": [{\"name\": \"Joe\"}] }",
            "{\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\"}]}");

    TEST(   "{\"manyany_type\": [ \"NONE\" ], \"manyany\": [ null ] }",
            "{\"manyany_type\":[\"NONE\"],\"manyany\":[null]}");

    TEST(   "{\"manyany_type\": [ \"Monster\", \"NONE\" ], \"manyany\": [{\"name\": \"Joe\"}, null] }",
            "{\"manyany_type\":[\"Monster\",\"NONE\"],\"manyany\":[{\"name\":\"Joe\"},null]}");

    TEST(   "{\"manyany_type\": [ \"Monster\" ], \"manyany\": [ { \"name\":\"Joe\", \"test_type\": \"Monster\", \"test\": { \"name\": \"any Monster\" } } ] }",
            "{\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\",\"test_type\":\"Monster\",\"test\":{\"name\":\"any Monster\"}}]}");

    TEST(   "{\"manyany\": [{\"name\": \"Joe\"}], \"manyany_type\": [ \"Monster\" ] }",
            "{\"manyany_type\":[\"Monster\"],\"manyany\":[{\"name\":\"Joe\"}]}");

    TEST(   "{\"manyany\": [{\"manyany\":[null, null], \"manyany_type\": [\"NONE\", \"NONE\"]}], \"manyany_type\": [ \"Alt\" ] }",
            "{\"manyany_type\":[\"Alt\"],\"manyany\":[{\"manyany_type\":[\"NONE\",\"NONE\"],\"manyany\":[null,null]}]}");

    END_TEST();
}

int fixed_array_tests(void)
{
    BEGIN_TEST(Alt);
    /* Fixed array */

#if UQ
    TEST(   "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0, 0, 0, 0, 0, 0,"
            " 0, 0, 0, 0, 0, 0, 0, 0, 16.0], col:[\"Blue Red\", Green, Red],"
            "tests:[ {b:4}, {a:1, b:2}],"
            " \"bar\": [ 100, 0, 0, 0, 0, 0, 0, 0, 0, 1000],"
            " \"text\":\"hello\"}}",
            "{\"fixed_array\":{\"foo\":[1,2,0,0,0,0,0,"
            "0,0,0,0,0,0,0,0,16],"
            "\"bar\":[100,0,0,0,0,0,0,0,0,1000],"
            "\"col\":[\"Red Blue\",\"Green\",\"Red\"],"
            "\"tests\":[{\"a\":0,\"b\":4},{\"a\":1,\"b\":2}],"
            "\"text\":\"hello\"}}");
#else
    TEST(   "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0, 0, 0, 0, 0, 0,"
            " 0, 0, 0, 0, 0, 0, 0, 0, 16.0], \"col\":[\"Blue Red\", \"Green\", \"Red\"],"
            "\"tests\":[ {\"b\":4}, {\"a\":1, \"b\":2}],"
            " \"bar\": [ 100, 0, 0, 0, 0, 0, 0, 0, 0, 1000],"
            " \"text\":\"hello\"}}",
            "{\"fixed_array\":{\"foo\":[1,2,0,0,0,0,0,"
            "0,0,0,0,0,0,0,0,16],"
            "\"bar\":[100,0,0,0,0,0,0,0,0,1000],"
            "\"col\":[\"Red Blue\",\"Green\",\"Red\"],"
            "\"tests\":[{\"a\":0,\"b\":4},{\"a\":1,\"b\":2}],"
            "\"text\":\"hello\"}}");
#endif

    TEST_FLAGS(flatcc_json_parser_f_skip_array_overflow, 0,
            "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0, 0, 0, 0, 0, 0,"
            " 0, 0, 0, 0, 0, 0, 0, 0, 16.0, 99],"
            " \"bar\": [ 100, 0, 0, 0, 0, 0, 0, 0, 0, 1000, 99],"
            " \"text\":\"hello, world\"}}",
            "{\"fixed_array\":{\"foo\":[1,2,0,0,0,0,0,"
            "0,0,0,0,0,0,0,0,16],"
            "\"bar\":[100,0,0,0,0,0,0,0,0,1000],"
            "\"col\":[0,0,0],"
            "\"tests\":[{\"a\":0,\"b\":0},{\"a\":0,\"b\":0}],"
            "\"text\":\"hello\"}}");

    TEST(   "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0 ],"
            " \"bar\": [ 100 ], \"text\": \"K\\x00A\\x00\" }}",
            "{\"fixed_array\":{\"foo\":[1,2,0,0,0,0,0,"
            "0,0,0,0,0,0,0,0,0],"
            "\"bar\":[100,0,0,0,0,0,0,0,0,0],"
            "\"col\":[0,0,0],"
            "\"tests\":[{\"a\":0,\"b\":0},{\"a\":0,\"b\":0}],"
            "\"text\":\"K\\u0000A\"}}");

    TEST_ERROR_FLAGS(flatcc_json_parser_f_reject_array_underflow, 0,
            "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0 ] }}",
            flatcc_json_parser_error_array_underflow);

    TEST_ERROR_FLAGS(flatcc_json_parser_f_reject_array_underflow, 0,
            "{ \"fixed_array\": { \"text\": \"K\\x00A\\x00\" }}",
            flatcc_json_parser_error_array_underflow);

    TEST_ERROR(
            "{ \"fixed_array\": { \"foo\": [ 1.0, 2.0, 0, 0, 0, 0, 0,"
            " 0, 0, 0, 0, 0, 0, 0, 0, 16.0, 99],"
            " \"bar\": [ 100, 0, 0, 0, 0, 0, 0, 0, 0, 1000, 99] }}",
            flatcc_json_parser_error_array_overflow);

    END_TEST();
}

/*
 * Here we cover some border cases around unions and flag
 * enumerations, and nested buffers.
 *
 * More complex objects with struct members etc. are reasonably
 * covered in the printer and parser tests using the golden data
 * set.
 */
int main(void)
{
    BEGIN_TEST(Monster);

    ret |= edge_case_tests();
    ret |= error_case_tests();
    ret |= union_vector_tests();
    ret |= fixed_array_tests();
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
            "{\"name\":\"Monster\",\"testempty\":{}}");

    /* Test empty array */
    TEST(   "{ \"name\": \"Monster\", \"testarrayoftables\": [] }",
            "{\"name\":\"Monster\",\"testarrayoftables\":[]}");

    /* Test JSON prefix parsing */
    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\": { \"aaaa\": \"test\", \"aaaa12345\": 17 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\":{\"aaaa\":\"test\",\"aaaa12345\":17}}}}");

    /* TODO: this parses with the last to }} missing, although it does not add the broken objects. */
    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\": { \"bbbb\": \"test\", \"bbbb1234\": 19 } }",
            "{\"name\":\"Monster\"}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\": { \"bbbb\": \"test\", \"bbbb1234\": 19 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\":{\"bbbb\":\"test\",\"bbbb1234\":19}}}}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\": { \"cccc\": \"test\", \"cccc1234\": 19, \"cccc12345\": 17 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\":{\"cccc\":\"test\",\"cccc1234\":19,\"cccc12345\":17}}}}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\": { \"dddd1234\": 19, \"dddd12345\": 17 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing\":{\"dddd1234\":19,\"dddd12345\":17}}}}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing2\": { \"aaaa_bbbb_steps\": 19, \"aaaa_bbbb_start_\": 17 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing2\":{\"aaaa_bbbb_steps\":19,\"aaaa_bbbb_start_\":17}}}}");

    TEST(   "{ \"name\": \"Monster\", \"test_type\":\"Alt\", \"test\":{\"prefix\":{"
            "\"testjsonprefixparsing3\": { \"aaaa_bbbb_steps\": 19, \"aaaa_bbbb_start_steps\": 17 } }}}",
            "{\"name\":\"Monster\",\"test_type\":\"Alt\",\"test\":{\"prefix\":{"
            "\"testjsonprefixparsing3\":{\"aaaa_bbbb_steps\":19,\"aaaa_bbbb_start_steps\":17}}}}");

    return ret ? -1: 0;
}
