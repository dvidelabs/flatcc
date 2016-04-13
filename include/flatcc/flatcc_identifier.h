#ifndef FLATCC_IDENTIFIER_H
#define FLATCC_IDENTIFIER_H

#ifndef FLATCC_FLATBUFFERS_H
#error "include via flatcc/flatcc_flatbuffers.h"
#endif

/*
 * FlatBuffers identifiers are normally specified by "file_identifer" in
 * the schema, but a standard hash of the fully qualified type name can
 * also be used. This file implements such a mapping, but the generated
 * headers also contain the necessary information for known types.
 */


/*
 * Returns the type hash of a given name in native endian format.
 * Generated code already provides these, but if a name was changed
 * in the schema it may be relevant to recompute the hash manually.
 *
 * The wire-format of this value should always be little endian.
 *
 * Note: this must be the fully qualified name, e.g. in the namespace
 * "MyGame.Example":
 *
 *     flatbuffers_type_hash_from_name("MyGame.Example.Monster");
 *
 * or, in the global namespace just:
 *
 *     flatbuffers_type_hash_from_name("MyTable");
 *
 * This assumes 32 bit hash type. For other sizes, other FNV-1a
 * constants would be required.
 *
 * Note that we reserve hash value 0 for missing or ignored value.
 */
static inline flatbuffers_thash_t flatbuffers_type_hash_from_name(const char *name)
{
    uint32_t hash = 2166136261UL;
    while (*name) {
        hash ^= (uint32_t)*name;
        hash = hash * 16777619UL;
        ++name;
    }
    if (hash == 0) {
        hash = 2166136261UL;
    }
    return hash;
}

/*
 * Type hash encoded as little endian file identifier string.
 * Note: if type hash is 0, the identifier should be null which
 * we cannot return in this interface.
 */
static inline void flatbuffers_identifier_from_type_hash(flatbuffers_thash_t type_hash, flatbuffers_fid_t out_identifier)
{
    out_identifier[0] = type_hash & 0xff;
    type_hash >>= 8;
    out_identifier[1] = type_hash & 0xff;
    type_hash >>= 8;
    out_identifier[2] = type_hash & 0xff;
    type_hash >>= 8;
    out_identifier[3] = type_hash & 0xff;
}

/* Native integer encoding of file identifier. */
static inline flatbuffers_thash_t flatbuffers_type_hash_from_identifier(const flatbuffers_fid_t identifier)
{
    uint8_t *p = (uint8_t *)identifier;

    return identifier ?
        (uint32_t)p[0] + (((uint32_t)p[1]) << 8) + (((uint32_t)p[2]) << 16) + (((uint32_t)p[3]) << 24) : 0;
}

/*
 * Computes the little endian wire format of the type hash. It can be
 * used as a file identifer argument to various flatcc buffer calls.
 *
 * `flatbuffers_fid_t` is just `char [4]` for the default flatbuffers
 * type system defined in `flatcc/flatcc_types.h`.
 */
static inline void flatbuffers_identifier_from_name(const char *name, flatbuffers_fid_t out_identifier)
{
    flatbuffers_identifier_from_type_hash(flatbuffers_type_hash_from_name(name), out_identifier);
}

/*
 * This is a collision free hash (a permutation) of the type hash to
 * provide better distribution for use in hash tables. It is likely not
 * necessary in praxis, and for uniqueness of identifiers at provides no
 * advantage over just using the FNV-1a type hash, except when truncating
 * the identifier to less than 32-bits.
 */
static inline uint32_t flatbuffers_disperse_type_hash(flatbuffers_thash_t type_hash)
{
    /* http://stackoverflow.com/a/12996028 */
    uint32_t x = type_hash;

    x = ((x >> 16) ^ x) * 0x45d9f3bUL;
    x = ((x >> 16) ^ x) * 0x45d9f3bUL;
    x = ((x >> 16) ^ x);
    return x;
}


/* We have hardcoded assumptions about identifier size. */
static_assert(sizeof(flatbuffers_fid_t) == 4, "unexpected file identifier size");
static_assert(sizeof(flatbuffers_thash_t) == 4, "unexpected type hash size");

#endif /* FLATCC_IDENTIFIER_H */
