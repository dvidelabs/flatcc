#include "flatcc/support/readfile.h"
#include "flatcc/reflection/reflection_reader.h"
#include "flatcc/portable/pcrt.h"

/* -DFLATCC_PORTABLE may help if inttypes.h is missing. */
#ifndef PRId64
#include <inttypes.h>
#endif


/* This is not an exhaustive test. */
int test_schema(const char *monster_bfbs)
{
    void *buffer;
    size_t size;
    int ret = -1;
    reflection_Schema_table_t S;
    reflection_Object_vec_t Objs;
    reflection_Object_table_t Obj;
    reflection_Field_vec_t Flds;
    reflection_Field_table_t F;
    reflection_Type_table_t T;
    size_t k, monster_index;
    reflection_Service_vec_t Svcs;
    reflection_Service_table_t Svc;
    reflection_RPCCall_vec_t Calls;
    reflection_RPCCall_table_t Call;
    size_t call_index;
    const char *strval;

    buffer = readfile(monster_bfbs, 100000, &size);
    if (!buffer) {
        printf("failed to load binary schema\n");
        goto done;
    }
    S = reflection_Schema_as_root(buffer);
    Objs = reflection_Schema_objects(S);
    for (k = 0; k < reflection_Object_vec_len(Objs); ++k) {
        printf("dbg: obj #%d : %s\n", (int)k,
                reflection_Object_name(reflection_Object_vec_at(Objs, k)));
    }
    k = reflection_Object_vec_find(Objs, "MyGame.Example.Monster");
    if (k == flatbuffers_not_found) {
        printf("Could not find monster in schema\n");
        goto done;
    }
    monster_index = k;
    Obj = reflection_Object_vec_at(Objs, k);
    if (strcmp(reflection_Object_name(Obj), "MyGame.Example.Monster")) {
        printf("Found wrong object in schema\n");
        goto done;
    }
    Flds = reflection_Object_fields(Obj);
    k = reflection_Field_vec_find(Flds, "mana");
    if (k == flatbuffers_not_found) {
        printf("Did not find mana field in Monster schema\n");
        goto done;
    }
    F = reflection_Field_vec_at(Flds, k);
    if (reflection_Field_default_integer(F) != 150) {
        printf("mana field has wrong default value\n");
        printf("field name: %s\n", reflection_Field_name(F));
        printf("%"PRId64"\n", (int64_t)reflection_Field_default_integer(F));
        goto done;
    }
    T = reflection_Field_type(F);
    if (reflection_Type_base_type(T) != reflection_BaseType_Short) {
        printf("mana field has wrong type\n");
        goto done;
    }
    if (reflection_Field_optional(F)) {
        printf("mana field is not optional\n");
        goto done;
    }
    k = reflection_Field_vec_find(Flds, "enemy");
    if (k == flatbuffers_not_found) {
        printf("enemy field not found\n");
        goto done;
    }
    T = reflection_Field_type(reflection_Field_vec_at(Flds, k));
    if (reflection_Type_base_type(T) != reflection_BaseType_Obj) {
        printf("enemy is not an object\n");
        goto done;
    }
    if (reflection_Type_index(T) != (int32_t)monster_index) {
        printf("enemy is not a monster\n");
        goto done;
    }
    k = reflection_Field_vec_find(Flds, "testarrayoftables");
    if (k == flatbuffers_not_found) {
        printf("array of tables not found\n");
        goto done;
    }
    T = reflection_Field_type(reflection_Field_vec_at(Flds, k));
    if (reflection_Type_base_type(T) != reflection_BaseType_Vector) {
        printf("array of tables is not of vector type\n");
        goto done;
    }
    if (reflection_Type_element(T) != reflection_BaseType_Obj) {
        printf("array of tables is not a vector of table type\n");
        goto done;
    }
    if (reflection_Type_index(T) != (int32_t)monster_index) {
        printf("array of tables is not a monster vector\n");
        goto done;
    }
    /* list services and calls */
    Svcs = reflection_Schema_services(S);
    for (k = 0; k < reflection_Service_vec_len(Svcs); ++k) {
        Svc = reflection_Service_vec_at(Svcs, k);
        printf("dbg: svc #%d : %s\n", (int)k,
                reflection_Service_name(Svc));
        Calls = reflection_Service_calls(Svc);
        for (call_index = 0 ;
             call_index < reflection_RPCCall_vec_len(Calls) ;
             call_index++) {
            Call = reflection_RPCCall_vec_at(Calls, call_index);
            printf("dbg:    call %d : %s\n", (int)call_index,
                reflection_RPCCall_name(Call));
        }
    }
    /* Within service MyGame.Example.MonsterStorage ... */
    k = reflection_Service_vec_find(Svcs, "MyGame.Example.MonsterStorage");
    if (k == flatbuffers_not_found) {
        printf("Could not find MonsterStorage service in schema\n");
        goto done;
    }
    Svc = reflection_Service_vec_at(Svcs, k);
    /* ... search the RPC call Store */
    Calls = reflection_Service_calls(Svc);
    k = reflection_RPCCall_vec_find(Calls, "Store");
    if (k == flatbuffers_not_found) {
        printf("Could not find call Store in service\n");
        goto done;
    }
    Call = reflection_RPCCall_vec_at(Calls, k);
    /* Ensure request type is MyGame.Example.Monster */
    Obj = reflection_Object_vec_at(Objs, monster_index);
    if (Obj != reflection_RPCCall_request(Call)) {
        printf("Wrong request type of rpc call\n");
        goto done;
    }
    /* Ensure response type is MyGame.Example.Stat */
    k = reflection_Object_vec_find(Objs, "MyGame.Example.Stat");
    if (k == flatbuffers_not_found) {
        printf("Could not find Stat in schema\n");
        goto done;
    }
    Obj = reflection_Object_vec_at(Objs, k);
    if (Obj != reflection_RPCCall_response(Call)) {
        printf("Wrong response type of rpc call\n");
        goto done;
    }
    /* check the call has an attribute "streaming" */
    k = reflection_KeyValue_vec_scan(reflection_RPCCall_attributes(Call), "streaming");
    if (k == flatbuffers_not_found) {
        printf("Could not find attribute in call\n");
        goto done;
    }
    /* check the attribute value is "none" */
    strval = reflection_KeyValue_value(
                reflection_KeyValue_vec_at(reflection_RPCCall_attributes(Call), k));
    if (!strval || 0 != strcmp("none", strval)) {
        printf("Wrong attribute value in call\n");
        goto done;
    }
    ret = 0;
done:
    if (buffer) {
        free(buffer);
    }
    return ret;
}

/* We take arguments so test can run without copying sources. */
#define usage \
"wrong number of arguments:\n" \
"usage: <program> [<output-filename>]\n"

const char *filename = "generated/monster_test.bfbs";

int main(int argc, char *argv[])
{
    /* Avoid assert dialogs on Windows. */
    init_headless_crt();

    if (argc != 1 && argc != 2) {
        fprintf(stderr, usage);
        exit(1);
    }
    if (argc == 2) {
        filename = argv[1];
    }

    return test_schema(filename);
}
