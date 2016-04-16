#include <stdio.h>
#include "monster_test_json_parser.h"
#include "monster_test_json_printer.h"
#include "monster_test_verifier.h"

#undef ns
#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)

int test_json(char *json, char *expect, int parse_flags, int print_flags, int line)
{
    int ret = -1;
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

    if ((ret = monster_test_parse_json(B, &parser, json, strlen(json), parse_flags))) {
        fprintf(stderr, "%d: json test: parse failed: %s\n", line, flatcc_json_parser_error_string(ret));
        fprintf(stderr, "%s\n", json);
        for (i = 0; i < parser.pos - 1; ++i) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "^\n");
        goto failed;
    }
    flatbuffer = flatcc_builder_finalize_buffer(B, &flatbuffer_size);
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
        free(flatbuffer);
    }
    flatcc_builder_clear(B);
    flatcc_json_printer_clear(&printer);
    return ret;
failed:
    ret = -1;
    goto done;
}

#define TEST(x, y) \
    ret |= test_json((x), (y), 0, 0, __LINE__);

#define TEST_FLAGS(fparse, fprint, x, y) \
    ret |= test_json((x), (y), (fparse), (fprint), __LINE__);

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
    TEST(   "{ name: \"Monster\", color: \"Green Blue Red Blue\"}",
            "{\"name\":\"Monster\",\"color\":19}");
#endif


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

#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST 
    TEST(   "{ name: \"Monster\", color: Green Red }",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");
#endif

#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST 
    /* No leading space in unquoted flag. */
    TEST(   "{ name: \"Monster\", color:Green Red }",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");

    TEST(   "{ name: \"Monster\", color: Green Red}",
            "{\"name\":\"Monster\",\"color\":\"Red Green\"}");

    TEST(   "{ name: \"Monster\", color:Green   Blue Red   }",
            "{\"name\":\"Monster\",\"color\":\"Red Green Blue\"}");
#endif

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

#if FLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST
    /*
     * Test that generic parsing handles multiple flags correctly during
     * first pass before backtracking.
     */
    TEST(   "{ name: \"Monster\", test: { name: \"second Monster\", color: Red Green }, test_type: Monster  }",
            "{\"name\":\"Monster\",\"test_type\":\"Monster\",\"test\":{\"name\":\"second Monster\",\"color\":\"Red Green\"}}");
#endif

    /* Ditto quoted flags. */
    TEST(   "{ name: \"Monster\", test: { name: \"second Monster\", color: \" Red Green \" }, test_type: Monster  }",
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
    TEST(   "{ name: \"Mon\xfF\xFf\\x03s\\xC3\\xF9\\u00F8ter\\b\\f\\n\\r\\t\\\"\\\\\\/'/\", }",
            "{\"name\":\"Mon\xff\xff\\u0003s\xc3\xf9\xc3\xb8ter\\b\\f\\n\\r\\t\\\"\\\\/'/\"}");

    TEST(   "{ name: \"\\u168B\\u1691\"}",
            "{\"name\":\"\xe1\x9a\x8b\xe1\x9a\x91\"}");

    /* Nested flatbuffer, either is a known object, or as a vector. */
    TEST(   "{ name: \"Monster\", testnestedflatbuffer:{ \"name\": \"sub Monster\" } }",
            "{\"name\":\"Monster\",\"testnestedflatbuffer\":{\"name\":\"sub Monster\"}}");

    TEST(   "{ name: \"Monster\", testnestedflatbuffer:"
            "[" /* start of nested flatbuffer, implicit size: 40 */
            "4,0,0,0," /* header: object offset = 4, no identifier */
            "248,255,255,255," /* vtable offset */
            "16,0,0,0," /* offset to name */
            "12,0,8,0,0,0,0,0,0,0,4,0," /* vtable */
            "11,0,0,0,115,117,98,32,77,111,110,115,116,101,114,0" /* name = "sub Monster" */
            "]" /* end of nested flatbuffer */
            "}",
            "{\"name\":\"Monster\",\"testnestedflatbuffer\":{\"name\":\"sub Monster\"}}");

    return ret ? -1: 0;
}
