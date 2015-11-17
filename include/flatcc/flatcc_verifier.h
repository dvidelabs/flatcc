#ifndef FLATCC_VERIFIER_H
#define FLATCC_VERIFIER_H

/*
 * Runtime support for verifying flatbuffers.
 *
 * Link with the verifier implementation file.
 *
 * Note:
 *
 * 1) nested buffers will NOT have their identifier verified.
 * The user may do so subsequently. The reason is in part because
 * the information is not readily avaible without generated reader code,
 * in part because the buffer might use a different, but valid,
 * identifier and the user has no chance of specifiying this in the
 * verifier code. The root verifier also doesn't assume a specific id
 * but accepts a user supplied input which may be null.
 *
 * 2) All offsets in a buffer are verified for alignment relative to the
 * buffer start, but the buffer itself is only assumed to aligned to
 * uoffset_t. A reader should therefore ensure buffer alignment separately
 * before reading the buffer. Nested buffers are in fact checked for
 * alignment, but still only relative to the root buffer.
 *
 * 3) The max nesting level includes nested buffer nestings, so the
 * verifier might fail even if the individual buffers are otherwise ok.
 * This is to prevent abuse with lots of nested buffers.
 *
 *
 * IMPORTANT:
 *
 * Even if verifier passes, the buffer may be invalid to access due to
 * lack of alignemnt in memory, but the verifier is safe to call.
 *
 * NOTE: The buffer is not safe to modify after verification because an
 * attacker may craft overlapping data structures such that modification
 * of one field updates another in a way that violates the buffer
 * constraints. This may also be caused by a clever compression scheme.
 *
 * It is likely faster to rewrite the table although this is also
 * dangerous because an attacker (or even normal user) can draft a DAG
 * that explodes when expanded carelesslessly. A safer approach is to
 * hash all object references written and reuse those that match. This
 * will expand references into other objects while bounding expansion
 * and it will be safe to update assuming shared objects are ok to
 * update.
 *
 */

#include "flatcc/flatcc_types.h"

/*
 * Type specific table verifier function that checks each known field
 * for existence in the vtable and then calls the appropriate verifier
 * function in this library.
 *
 * The table descriptor values have been verified for bounds, overflow,
 * and alignment, but vtable entries after header must be verified
 * for all fields the table verifier function understands.
 *
 * Calls other typespecific verifier functions recursively whenever a
 * table field, union or table vector is encountered.
 */
typedef struct flatcc_table_verifier_descriptor flatcc_table_verifier_descriptor_t;
struct flatcc_table_verifier_descriptor {
    /* Pointer to buffer. Not assumed to be aligned beyond uoffset_t. */
    const void *buf;
    /* Vtable of current table. */
    const void *vtable;
    /* Buffer size. */
    flatbuffers_uoffset_t end;
    /* Table offset relative to buffer start */
    flatbuffers_uoffset_t table;
    /* Table end relative to buffer start as per vtable[1] field. */
    flatbuffers_voffset_t tsize;
    /* Size of vtable in bytes. */
    flatbuffers_voffset_t vsize;
    /* Time to live: number nesting levels left before failure. */
    int ttl;
};

typedef int flatcc_table_verifier_f(flatcc_table_verifier_descriptor_t *td);

typedef int flatcc_union_verifier_f(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, uint8_t type);


/*
 * The `as_root` functions are normally the only functions called
 * explicitly in this interface.
 *
 * If `fid` is null, the identifier is not checked and is allowed to be entirely absent.
 *
 * The buffer must at least be aligned to uoffset_t on systems that
 * require aligned memory addresses. The buffer pointers alignment is
 * not significant to internal verification of the buffer.
 */
int flatcc_verify_struct_as_root(const void *buf, size_t bufsiz, const char *fid,
        uint16_t align, size_t size);
int flatcc_verify_table_as_root(const void *buf, size_t bufsiz, const char *fid,
        flatcc_table_verifier_f *root_tvf);

/*
 * The buffer header is verified by any of the `_as_root` verifiers, but
 * this function may be used as a quick sanity check.
 */
int flatcc_verify_buffer_header(const void *buf, size_t bufsiz, const char *fid);

/*
 * The following functions are typically called by a generated table
 * verifier function.
 */

/* Scalar, enum or struct field. */
int flatcc_verify_field(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, uint16_t align, size_t size);
/* Vector of scalars, enums or structs. */
int flatcc_verify_vector_field(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required, uint16_t align, size_t elem_size, size_t max_count);
int flatcc_verify_string_field(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required);
int flatcc_verify_string_vector_field(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required);
int flatcc_verify_table_field(flatcc_table_verifier_descriptor_t *td,
    flatbuffers_voffset_t id, int required, flatcc_table_verifier_f tvf);
int flatcc_verify_table_vector_field(flatcc_table_verifier_descriptor_t *td,
    flatbuffers_voffset_t id, int required, flatcc_table_verifier_f tvf);

/* Table verifiers pass 0 as fid. */
int flatcc_verify_struct_as_nested_root(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required, const char *fid,
        uint16_t align, size_t size);
int flatcc_verify_table_as_nested_root(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required, const char *fid,
        uint16_t align, flatcc_table_verifier_f tvf);

/*
 * A NONE type will not accept a table being present, and a required
 * union will not accept a type field being absent, and an absent type
 * field will not accept a table field being present.
 *
 * If the above checks out and the type is not NONE, the uvf callback
 * is executed. It must test each known table type and silently accept
 * any unknown table type for forward compatibility. A union table
 * member is verified without the required flag because an absent table
 * encodes a typed NULL value while an absent type field encodes a
 * missing union which fails if required.
 */
int flatcc_verify_union_field(flatcc_table_verifier_descriptor_t *td,
        flatbuffers_voffset_t id, int required, flatcc_union_verifier_f *uvf);

#endif /* FLATCC_VERIFIER_H */
