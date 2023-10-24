/*
 * Parse and verify schema complex nonsense schema, then generate common
 * and schema specific files for reader and builder, all outfileenated to
 * stdout, followed by the schema source in a comment.
 *
 * Notes:
 *
 * Later type declarations are visible in the same file regardles of
 * namespace. Earlier type declarations in included files are also
 * visible. Included files cannot see types in later included or
 * including files. (We do not test file inclusion here though). This
 * behaviour is chosen both because it seems sensible and because it
 * allows for independent file generation of each schema file.
 *
 * Googles flatc compiler does in some cases allow for later references
 * e.g. Monster reference itself, but require structs to be ordered.
 * We do not required that so structs are sorted topologically before
 * being given to the code generator. Tables use multiple layers of
 * forward declarations in the generated C code.
 *
 * Note: The google flatc compiler does support multiple namspaces
 * but apparently it not currently generate C++ correctly. As such is
 * not well-defined what exactly the meaning of a namespace is. We have
 * chosen it to mean everything from the definition until the next in
 * the same file. Anything before a namespace declaration is global.
 * Namespace declarations only affect the local file. Namespace
 * references allow spaces between dots when used as an operator, but
 * not when used as a string attribute - this matches flatc behavior.
 * When a namespace prefix is not specified, but the current scope is
 * searched first, then the global scope.
 *
 * There is no way to specify the global scope once a namespace has been
 * declared, other than starting a new file. It could be considered to
 * allow for "namespace ;".
 *
 * The flatc compiler does not allow a namespace prefixes in root_type,
 * but that is likely an oversight. We support it.
 *
 */
#include <stdio.h>
#include <string.h>
#include "flatcc/flatcc.h"

int main(void)
{
    const char *name = "../xyzzy.fbs";

    char input[] =
        "\t//buffers do not support include statements\n"
        "//\tinclude \"foobar.fbs\";\n"
        "// in flatc, only one field can have a key, but we have no issues\n"
        "// as long as the vector is sorted accordingly. The first key gets\n"
        "// gets a shorter find alias method.\n"
        "// (all scalar vectors can also be searched - they have find defined)\n"
        "/* block comments are also allowed.\n */\n"
        "//////////////////////////////////////////////////////////\n"
        "//////////////////////////////////////////////////////////\n"
        "table point { x : float (key); y: float; z: float (key); }\n"
        "namespace mystic;\n"
        "/************ ANOTHER DOC CASE *************/\n"
        "table island { lattitude : int; longitude : int; }\n"
        "     /// There are two different point tables\n"
        "// we test multi line doc comments here - this line should be ignored.\n"
        " /// - one in each name space.\n"
        "table point { interest: agent; blank: string; geo: mystic.island; }\n"
        "enum agent:int { lot, pirate, vessel, navy, parrot }\n"
        "\tnamespace the;\n"
        "//root_type point;\n"
        "attribute \"foo\";\n"
        "//attribute \"\"; // ensure empty strings are accepted.\n"
        "/// shade is for CG applications\n"
        "struct shade (force_align:2) { x: byte; y: byte; z: byte;\n"
        "/// alpha is unsigned!\n"
        "alpha: ubyte (key); }\n"
        "/// the.ui is a union\n"
        "///\n"
        "/// We got one blank comment line above.\n"
        "union u1 { /// Note that the.point is different from mystic.point in other namespace.\n"
        "point\n"
        "= /// meaningless doc comment that should be stripped\n"
        "2, foo = 4, mystic.island = 17, } enum e1:short { z = -2, one , two , three = 3, }\n"
        "// key on enum not supported by flatc\n"
        "table foo  { m: u1; e: e1 = z (key); x : int = mystic.agent.vessel; interest: mystic.agent = pirate; strange: mystic.agent = flags2.zulu; }\n"
        "// Unknown attributes can be repeated with varying types since behavior is unspecified.\n"
        "enum flags : short (bit_flags, \n"
        "/// foos are plentiful - here it is an enum of value 42\n"
        "foo: 42, foo, foo: \"hello\") { f1 = 1, f2 = 13, f3 }\n"
        "enum flags2 : uint (bit_flags) { zulu, alpha, bravo, charlie, delta, echo, foxtrot }\n"
        "/// A boolean enum - all enums must be type.\n"
        "enum confirm : bool { no, yes }\n"
        "// enums are not formally permitted in structs, but can be enabled.\n"
        "// This is advanced: boolean enum binary search on struct vector ...\n"
        "struct notify { primary_recipient: confirm (key); secondary_recipient: confirm; flags : flags; }\n"
        "// duplicates are disallowed by default, but can be enabled\n"
        "// enum dupes : byte { large = 2, great = 2, small = 0, other }\n"
        "table goo { hello: string (key, required); confirmations: [confirm];\n"
        "            never_mind: double = 3.1415 (deprecated);\n"
        "            embedded_t: [ubyte] (nested_flatbuffer: \"foo\");\n"
        "            embedded_s: [ubyte] (nested_flatbuffer: \"little.whale.c2\");\n"
        "            shady: shade;\n"
        "}\n"
        "struct s1 (force_align:4) { index: int (key); }\n"
        "struct c1 { a: ubyte; x1 : little.whale.c2; x2:uint; x3: short; light: shade (deprecated); }\n"
        "/// not all constructs support doc comments - this one doesn't\n"
        "namespace little.whale;\n"
        "struct c2 { y : c3; }\n"
        "//struct c3 { z : c1; }\n"
        "struct c3 { z : the.s1; }\n"
        "file_identifier \"fbuz\";\n"
        "file_extension \"cgen_test\";\n"
        "root_type little.whale.c2;\n"
        "//root_type c2;\n"
        "//root_type the.goo;\n"
        "table hop { einhorn: c3 (required); jupiter: c2; names: [string] (required); ehlist: [c3]; k2: the.goo; k2vec: [the.goo]; lunar: the.flags2 = bravo; }\n"
        "table TestOrder { x0 : byte; x1: bool = true; x2: short; x3: the.shade; x4: string; x5 : the.u1; x6 : [string]; x7: double; }\n"
        "table TestOrder2 (original_order) { x0 : byte; x1: bool = true; x1a : bool = 1; x2: short; x3: the.shade; x4: string; x5: the.u1; x6 : [string]; x7: double; }\n"
        "table StoreResponse {}\n"
        "rpc_service MonsterStorage {\n"
        "  Store(Monster):StoreResponse;\n"
        "  Retrieve(MonsterId):Monster;\n"
        "  RetrieveOne(MonsterId):Monster (deprecated);\n"
        "}\n"
        "/* \n"
        "*/table Monster {}\ntable MonsterId{ id: int; }\n"
        "/* \t/ */\n"; /* '\v' would give an error. */

    flatcc_options_t opts;
    flatcc_context_t ctx = 0;
    int ret = -1;

    flatcc_init_options(&opts);
    opts.cgen_common_reader = 1;
    opts.cgen_reader = 1;
    opts.cgen_common_builder = 1;
    opts.cgen_builder = 1;
    opts.gen_stdout = 1;

    /* The basename xyzzy is derived from path. */
    if (!(ctx = flatcc_create_context(&opts, name, 0, 0))) {
        fprintf(stderr, "unexpected: could not initialize context\n");
        return -1;
    }
    if ((ret = flatcc_parse_buffer(ctx, input, sizeof(input)))) {
        fprintf(stderr, "sorry, parse failed\n");
        goto done;
    } else {
        fprintf(stderr, "schema is valid!\n");
        fprintf(stderr, "now generating code for C ...\n\n");
        if (flatcc_generate_files(ctx)) {
            fprintf(stderr, "failed to generate output for C\n");
            goto done;
        };
        fprintf(stdout,
                "\n#if 0 /* FlatBuffers Schema Source */\n"
                "%s\n"
                "#endif /* FlatBuffers Schema Source */\n",
                input);
    }
    ret = 0;
done:
    flatcc_destroy_context(ctx);
    return ret;
}
