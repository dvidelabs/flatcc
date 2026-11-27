/* Build, clone, and selectively rebuild a small FlatBuffer. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "clone_sample_builder.h"
#include "clone_sample_verifier.h"

/* The returned buffer belongs to the caller, independently of the builder. */
static void *make_source(size_t *size)
{
    flatcc_builder_t builder;
    CloneSample_Item_ref_t shared;
    int32_t values[] = { -3, 8, 13 };
    void *buffer = 0;

    if (flatcc_builder_init(&builder)) return 0;
    if (CloneSample_Document_start_as_root(&builder) ||
        CloneSample_Item_start(&builder) ||
        CloneSample_Item_name_create_str(&builder, "shared item") ||
        CloneSample_Item_weight_add(&builder, 7)) goto done;
    shared = CloneSample_Item_end(&builder);
    if (!shared ||
        CloneSample_Document_id_add(&builder, 42) ||
        CloneSample_Document_note_create_str(&builder, "original") ||
        CloneSample_Document_values_create(&builder, values, 3) ||
        CloneSample_Document_left_add(&builder, shared) ||
        CloneSample_Document_right_add(&builder, shared) ||
        !CloneSample_Document_end_as_root(&builder)) goto done;
    buffer = flatcc_builder_finalize_aligned_buffer(&builder, size);

done:
    flatcc_builder_clear(&builder);
    return buffer;
}

/* `source` stays valid until the clone has finished. */
static void *clone_document(const void *source, int preserve_sharing, size_t *size)
{
    flatcc_builder_t builder;
    flatcc_refmap_t map, *previous = 0;
    void *buffer = 0;

    if (flatcc_builder_init(&builder)) return 0;
    flatcc_refmap_init(&map);
    if (preserve_sharing) previous = flatcc_builder_set_refmap(&builder, &map);

    /* clone_as_root starts and ends the new root; it is not followed by end_as_root. */
    if (CloneSample_Document_clone_as_root(&builder,
            CloneSample_Document_as_root(source))) {
        buffer = flatcc_builder_finalize_aligned_buffer(&builder, size);
    }

    /* Detach the stack map before clearing it and the builder. */
    if (preserve_sharing) flatcc_builder_set_refmap(&builder, previous);
    flatcc_refmap_clear(&map);
    flatcc_builder_clear(&builder);
    return buffer;
}

/* Build a new root with one changed field and pick the other fields. */
static void *rebuild_with_new_id(const void *source, size_t *size)
{
    flatcc_builder_t builder;
    flatcc_refmap_t map, *previous;
    CloneSample_Document_table_t document = CloneSample_Document_as_root(source);
    void *buffer = 0;

    if (flatcc_builder_init(&builder)) return 0;
    flatcc_refmap_init(&map);
    previous = flatcc_builder_set_refmap(&builder, &map);
    if (CloneSample_Document_start_as_root(&builder) ||
        CloneSample_Document_id_add(&builder, CloneSample_Document_id(document) + 1) ||
        CloneSample_Document_note_pick(&builder, document) ||
        CloneSample_Document_values_pick(&builder, document) ||
        CloneSample_Document_left_pick(&builder, document) ||
        CloneSample_Document_right_pick(&builder, document) ||
        !CloneSample_Document_end_as_root(&builder)) goto done;
    buffer = flatcc_builder_finalize_aligned_buffer(&builder, size);

done:
    flatcc_builder_set_refmap(&builder, previous);
    flatcc_refmap_clear(&map);
    flatcc_builder_clear(&builder);
    return buffer;
}

static int check_document(const char *label, const void *buffer, size_t size,
        uint32_t expected_id, int expected_shared)
{
    CloneSample_Document_table_t document;
    CloneSample_Item_table_t left, right;
    flatbuffers_int32_vec_t values;
    const char *note, *left_name, *right_name;

    if (!buffer || CloneSample_Document_verify_as_root(buffer, size)) return -1;
    document = CloneSample_Document_as_root(buffer);
    note = CloneSample_Document_note(document);
    values = CloneSample_Document_values(document);
    left = CloneSample_Document_left(document);
    right = CloneSample_Document_right(document);
    if (!note || !values || !left || !right) return -1;
    left_name = CloneSample_Item_name(left);
    right_name = CloneSample_Item_name(right);
    if (!left_name || !right_name ||
        CloneSample_Document_id(document) != expected_id ||
        strcmp(note, "original") || flatbuffers_vec_len(values) != 3 ||
        flatbuffers_int32_vec_at(values, 0) != -3 ||
        flatbuffers_int32_vec_at(values, 1) != 8 ||
        flatbuffers_int32_vec_at(values, 2) != 13 ||
        strcmp(left_name, "shared item") || CloneSample_Item_weight(left) != 7 ||
        strcmp(right_name, "shared item") || CloneSample_Item_weight(right) != 7 ||
        ((left == right) != expected_shared)) return -1;

    printf("%s: id=%u, note=%s, values=[%d,%d,%d], "
           "left=(%s,%d), right=(%s,%d), shared=%s\n",
           label, (unsigned)CloneSample_Document_id(document), note,
           (int)flatbuffers_int32_vec_at(values, 0),
           (int)flatbuffers_int32_vec_at(values, 1),
           (int)flatbuffers_int32_vec_at(values, 2),
           left_name, (int)CloneSample_Item_weight(left),
           right_name, (int)CloneSample_Item_weight(right),
           left == right ? "yes" : "no");
    return 0;
}

int main(void)
{
    void *source = 0, *plain = 0, *mapped = 0, *updated = 0;
    size_t source_size = 0, plain_size = 0, mapped_size = 0, updated_size = 0;
    int result = 1;

    source = make_source(&source_size);
    if (check_document("source", source, source_size, 42, 1)) goto done;
    plain = clone_document(source, 0, &plain_size);
    if (check_document("clone without refmap", plain, plain_size, 42, 0)) goto done;
    mapped = clone_document(source, 1, &mapped_size);
    if (check_document("clone with refmap", mapped, mapped_size, 42, 1)) goto done;
    updated = rebuild_with_new_id(source, &updated_size);
    if (check_document("rebuilt", updated, updated_size, 43, 1) ||
        check_document("source after rebuild", source, source_size, 42, 1)) goto done;
    result = 0;

done:
    if (result) fprintf(stderr, "clone sample failed\n");
    flatcc_builder_aligned_free(source);
    flatcc_builder_aligned_free(plain);
    flatcc_builder_aligned_free(mapped);
    flatcc_builder_aligned_free(updated);
    return result;
}
