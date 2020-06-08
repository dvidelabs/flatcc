#include <stdio.h>
#include "monster_test_builder.h"
#include "flatcc/support/elapsed.h"

#define ns(x) FLATBUFFERS_WRAP_NAMESPACE(MyGame_Example, x)
#define nsc(x) FLATBUFFERS_WRAP_NAMESPACE(flatbuffers, x)
#define c_vec_len(V) (sizeof(V)/sizeof((V)[0]))

#define MEASURE_DECODE 1
#define MONSTER_REP 1000
#define NAME_REP 100
#define INVENTORY_REP 100

static uint8_t invdata[1000];

static ns(Monster_ref_t) create_monster(flatcc_builder_t *B)
{
    size_t i;

    ns(Monster_start(B));
    ns(Monster_name_start(B));
    for (i = 0; i < NAME_REP; ++i) {
        nsc(string_append(B, "Monster", 7));
    }
    ns(Monster_name_end(B));
    ns(Monster_inventory_start(B));
    for (i = 0; i < INVENTORY_REP; ++i) {
        nsc(uint8_vec_append(B, invdata, c_vec_len(invdata)));
    }
    ns(Monster_inventory_end(B));
    return ns(Monster_end(B));
}

static ns(Monster_vec_ref_t) create_monsters(flatcc_builder_t *B)
{
    size_t i;
    ns(Monster_ref_t) m;

    ns(Monster_vec_start(B));
    for (i = 0; i < MONSTER_REP; ++i) {
        m = create_monster(B);
        assert(m);
        ns(Monster_vec_push(B, m));
    }
    return ns(Monster_vec_end(B));
}

static int create_root_monster(flatcc_builder_t *B)
{
    ns(Monster_vec_ref_t) mv;

    flatcc_builder_reset(B);
    ns(Monster_start_as_root(B));
    ns(Monster_name_create_str(B, "root_monster"));
    mv = create_monsters(B);
    assert(mv);
    ns(Monster_testarrayoftables_add(B, mv));
    ns(Monster_end_as_root(B));
    return 0;
}

#if MEASURE_DECODE
static int verify_monster(const char *base, ns(Monster_table_t) mon)
{
    size_t i;
    nsc(string_t) s = ns(Monster_name(mon));
    /*
     * This only works because it is a byte, otherwise
     * vec_at should be used to convert endian format.
     */
    const uint8_t *inv = ns(Monster_inventory(mon));

    if (nsc(string_len(s)) != NAME_REP * 7) {
        assert(0);
        return -1;
    }
    if (nsc(uint8_vec_len(inv)) != INVENTORY_REP * c_vec_len(invdata)) {
        assert(0);
        return -1;
    }
    for (i = 0; i < NAME_REP; ++i) {
        if (memcmp(s + i * 7, "Monster", 7)) {
            printf("failed monster name at %lu: %s\n", (unsigned long)i, s ? s : "NULL");
            printf("offset: %ld\n", (long)(s + i * 7 - base));
            assert(0);
            return -1;
        }
    }
    for (i = 0; i < INVENTORY_REP; ++i) {
        if (memcmp(inv + i * c_vec_len(invdata), invdata, c_vec_len(invdata))) {
            assert(0);
            return -1;
        }
    }
    return 0;
}
#endif

int main(int argc, char *argv[])
{
    FILE *fp;
    void *buffer;
    size_t size;
    flatcc_builder_t builder, *B;
    ns(Monster_table_t) mon;
    ns(Monster_vec_t) mv;
    double t1, t2;
    int rep = 10, i;
    int ret = 0;

#if MEASURE_DECODE
    size_t j;
#endif

    (void)argc;
    (void)argv;

    B = &builder;
    flatcc_builder_init(B);
    create_root_monster(B);
    buffer = flatcc_builder_finalize_buffer(B, &size);
    fp = fopen("monster_load_test.dat", "wb");
    if (!fp) {
        ret = -1;
        goto done;
    }
    ret |= size != fwrite(buffer, 1, size, fp);
    fclose(fp);
    if (ret) {
        goto done;
    }
    printf("buffer size: %lu\n", (unsigned long)size);
    printf("start timing ...\n");
    t1 = elapsed_realtime();
    for (i = 0; i < rep; ++i) {
        create_root_monster(B);
        flatcc_builder_copy_buffer(B, buffer, size);
        mon = ns(Monster_as_root(buffer));
        ret |= strcmp(ns(Monster_name(mon)), "root_monster");
        assert(ret == 0);
        mv = ns(Monster_testarrayoftables(mon));
        /* Negated logic, 0 is OK. */
        ret |=  ns(Monster_vec_len(mv)) != MONSTER_REP;
        assert(ret == 0);
#if MEASURE_DECODE
        for (j = 0; j < MONSTER_REP; ++j) {
            ret |= verify_monster(buffer, ns(Monster_vec_at(mv, j)));
            assert(ret == 0);
        }
#endif
        if (ret) {
            goto done;
        }
    }
    t2 = elapsed_realtime();
    show_benchmark("encode and partially decode large buffer", t1, t2, size, rep, 0);
done:
    flatcc_builder_clear(B);
    free(buffer);
    if (ret) {
        printf("load test failed\n");
    }
    return ret;
}
