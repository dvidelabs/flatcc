/* Minimal test with all headers generated into a single file. */
#include "zzz_monster_test.h"

int main(int argc, char *argv[])
{
    int ret;
    void *buf;
    size_t size;
    flatcc_builder_t builder, *B;

    (void)argc;
    (void)argv;

    B = &builder;
    flatcc_builder_init(B);

    zzz_MyGame_Example_Monster_start_as_root(B);
    zzz_MyGame_Example_Monster_name_create_str(B, "MyMonster");
    zzz_MyGame_Example_Monster_end_as_root(B);
    buf = flatcc_builder_get_direct_buffer(B, &size);
    ret = zzz_MyGame_Example_Monster_verify_as_root_with_identifier(buf, size, zzz_MyGame_Example_Monster_file_identifier);
    flatcc_builder_clear(B);
    return ret;
}
