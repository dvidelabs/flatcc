#ifndef HT_HASH_FUNCTION_H
#define HT_HASH_FUNCTION_H

#include <stddef.h>

#ifndef HT_HASH_SEED
#define HT_HASH_SEED 0x2f693b52L
#endif

#ifndef HT_HASH_32

#include "cmetrohash.h"

static inline size_t ht_default_hash_function(const void *key, size_t len)
{
    uint64_t out;

    cmetrohash64_1((const uint8_t *)key, len, HT_HASH_SEED, (uint8_t *)&out);
    return (unsigned int)out;
}

/* When using the pointer directly as a hash key. */
static inline size_t ht_ptr_hash_function(const void *key, size_t len)
{
    /* MurmurHash3 64-bit finalizer */
    size_t x;

    (void)len;

    x= (size_t)key ^ (HT_HASH_SEED);

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdLL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53LL;
    x ^= x >> 33;
    return x;
}

#else

#include "PMurHash.h"

static inline size_t ht_default_hash_function(const void *key, size_t len)
{
    return (size_t)PMurHash32((HT_HASH_SEED), key, (int)len);
}

/* When using the pointer directly as a hash key. */
static inline size_t ht_ptr_hash_function(const void *key, size_t len)
{
    /* http://stackoverflow.com/a/12996028 */
    size_t x;

    x = (size_t)key ^ (HT_HASH_SEED);

    x = ((x >> 16) ^ x) * 0x45d9f3bL;
    x = ((x >> 16) ^ x) * 0x45d9f3bL;
    x = ((x >> 16) ^ x);
    return x;
}

#endif /* HT_HASH_32 */

/* Same as ht_ptr_hash_function but faster and better suited for low valued ints. */
static inline size_t ht_int_hash_function(const void *key, size_t len)
{
    (void)len;
    return ((size_t)key ^ (HT_HASH_SEED)) * 2654435761;
}

/* Bernsteins hash function, assumes string is zero terminated, len is ignored. */
static inline size_t ht_str_hash_function(const void *key, size_t len)
{
    const unsigned char *str = key;
    size_t hash = 5381 ^ (HT_HASH_SEED);
    int c;

    (void)len;

    while ((c = *str++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) xor c */

    return hash;
}

/* Hashes at most len characters or until zero terminatation. */
static inline size_t ht_strn_hash_function(const void *key, size_t len)
{
    const unsigned char *str = key;
    size_t hash = 5381 ^ (HT_HASH_SEED);
    int c;

    while (--len && (c = *str++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) xor c */

    return hash;
}

#endif /* HT_HASH_FUNCTION_H */
