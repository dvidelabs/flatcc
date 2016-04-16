/* Minimal test with all headers generated into a single file. */
#include "monster_test.h"

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

    MyGame_Example_Monster_start_as_root(B);
    MyGame_Example_Monster_name_create_str(B, "MyMonster");
    MyGame_Example_Monster_end_as_root(B);
    buf = flatcc_builder_get_direct_buffer(B, &size);
    ret = MyGame_Example_Monster_verify_as_root(buf, size);
    flatcc_builder_clear(B);
    return ret;
}
