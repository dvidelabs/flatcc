#ifndef DVSTD_HASH_H
#define DVSTD_HASH_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if !defined(DVSTD_HASH_FALLBACK)
/*
 * Currently xxhash 0.8.0 with XXH3 class hash functions is the clear leader.
 */
#include "xxhash.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__BIG_ENDIAN__) && !defined(DVSTD_HASH_BE)
#define DVSTD_HASH_BE 1
#endif

#if !defined(DVSTD_HASH_32) && !(UINTPTR_MAX > UINT32_MAX || ULONG_MAX > UINT32_MAX)
#define DVSTD_HASH_32 1
#endif

/*
 * Avoid 0 special case in hash functions and allow for
 * configuration with unguessable seed.
 *
 * If runtime control is desired, the seed can be the name of an
 * unsigned 32 or 64 bit variable.
 */

#ifndef DVSTD_HASH_SEED
#define DVSTD_HASH_SEED UINT32_C(0x2f693b52)
#define DVSTD_HASH_USES_DEFAULT_SEED 1
#else
#define DVSTD_HASH_USES_DEFAULT_SEED 0
#endif

/* Preferably use xxhash functions, but keep older functions for now. */
#ifndef DVSTD_HASH_FALLBACK

static inline size_t hash_mem(const void *key, size_t len)
{
#  if DVSTD_HASH_32
    return (size_t)XXH32(key, len, (unsigned int)(DVSTD_HASH_SEED));
#  else
#      if DVSTD_HASH_USES_DEFAULT_SEED
            /* This is slightly faster so do not impose a seed in the default case. */
           return (size_t)XXH3_64bits(key, len);
#      else
           return (size_t)XXH3_64bits_withSeed(key, len, (XXH64_hash_t)(DVSTD_HASH_SEED));
#      endif
#endif
}

#elif !defined(DVSTD_HASH_32)

#include "cmetrohash.h"
#define DVSTD_HASH_HAVE_METRO 1

static inline size_t hash_mem(const void *key, size_t len)
{
    uint64_t out;

    cmetrohash64_1((const uint8_t *)key, len, (uint64_t)DVSTD_HASH_SEED, (uint8_t *)&out);
    return (unsigned int)out;
}

#else /* DVSTD_HASH_32 */

#include "pmurhash.h"
#define DVSTD_HASH_HAVE_MURMUR 1

static inline size_t hash_mem(const void *key, size_t len)
{
    return (size_t)pmurhash32((uint32_t)(DVSTD_HASH_SEED), key, (int)len);
}

#endif /* DVSTD_HASH_32 */

static inline size_t hash_uint32(uint32_t key)
{
    uint32_t x = key + (uint32_t)(DVSTD_HASH_SEED);

    /* http://stackoverflow.com/a/12996028 */
    x = ((x >> 16) ^ x) * UINT32_C(0x45d9f3b);
    x = ((x >> 16) ^ x) * UINT32_C(0x45d9f3b);
    x = ((x >> 16) ^ x);
    return x;
}

static inline uint64_t hash_uint64(uint64_t key)
{
    uint64_t x = key + UINT64_C(0x9e3779b97f4a7c15) +
        (uint64_t)(DVSTD_HASH_SEED);

    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x;
}

static inline size_t hash_size_seed(size_t key, size_t seed)
{
#if SIZE_MAX > UINT32_MAX
    /* MurmurHash3 64-bit finalizer */
    uint64_t x;

    x = ((uint64_t)key) ^ (uint64_t)seed;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (size_t)x;
#else
    /* http://stackoverflow.com/a/12996028 */
    uint32_t x;

    x = ((uint32_t)(size_t)key) ^ (uint32_t)seed;

    x = ((x >> 16) ^ x) * 0x45d9f3bUL;
    x = ((x >> 16) ^ x) * 0x45d9f3bUL;
    x = ((x >> 16) ^ x);
    return x;
#endif
}

static inline size_t hash_size(size_t key)
{
    return hash_size_seed(key, DVSTD_HASH_SEED);
}

static inline size_t hash_ptr(const void *ptr)
{
    return hash_size((size_t)ptr);
}

static inline size_t hash_uint(unsigned int key)
{
#if UINT_MAX > UINT32_MAX
    return (size_t)hash_uint64((uint64_t)key);
#else
    return (size_t)hash_uint32((uint32_t)key);
#endif
}

static inline size_t hash_str(const char *key)
{
    return hash_mem(key, strlen(key));
}

static inline size_t hash_strn(const char *key, size_t len)
{
    const char *end;

    end = memchr (key, 0, len);
    len = end ? (size_t)(end - key) : len;
    return hash_mem(key, len);
}

/* Bernstains simple string hash - Hashes until zero terminated string. */
static inline size_t hash_str_b(const char *key)
{
    const unsigned char *str = (const unsigned char *)key;
    size_t hash = 5381 ^ (size_t)(DVSTD_HASH_SEED);
    int c;

    while ((c = *str++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) xor c */

    return hash;
}

/*
 * Bernsteins simple string hash - hashes until zero termination or
 * at most len characters whichever comes first.
 */
static inline size_t hash_strn_b(const char *key, size_t len)
{
    const unsigned char *str = (const unsigned char*)key;
    size_t hash = 5381 ^ (DVSTD_HASH_SEED);
    int c;

    while (--len && (c = *str++))
        hash = ((hash << 5) + hash) ^ c; /* (hash * 33) xor c */

    return hash;
}

static inline uint32_t hash_fnv1a32(const void *key, size_t len)
{
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint32_t prime = UINT32_C(0x1000193);
#endif
    uint32_t hash = UINT32_C(0x811c9dc5);
    const uint8_t *p = (const uint8_t *)key;

    while (len--) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 7) +
            (hash << 8) + (hash << 24);
#endif
    }
    return hash;
}

/* Hashes `len` characters that may contain zeroes. */
static inline uint64_t hash_fnv1a64(const void *key, size_t len)
{
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint64_t prime = UINT64_C(0x100000001b3);
#endif
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    const uint8_t *p = (const uint8_t *)key;

    while (len--) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 5) +
            (hash << 7) + (hash << 8) + (hash << 40);
#endif
    }
    return hash;
}

/* Hashes zero terminated string. */
static inline uint32_t hash_fnv1a32_str(const char *key)
{
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint32_t prime = UINT32_C(0x1000193);
#endif
    uint32_t hash = UINT32_C(0x811c9dc5);
    const uint8_t *p = (const uint8_t *)key;


    while (*p) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 7) +
            (hash << 8) + (hash << 24);
#endif
    }
    return hash;
}

/* Hashes zero terminated string. */
static inline uint64_t hash_fnv1a64_str(const char *key)
{
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint64_t prime = UINT64_C(0x100000001b3);
#endif
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    const uint8_t *p = (const uint8_t *)key;

    while (*p) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 5) +
            (hash << 7) + (hash << 8) + (hash << 40);
#endif
    }
    return hash;
}

static inline uint32_t hash_fnv1a32_update(uint32_t seed, uint8_t *buf, size_t len)
{
    uint8_t *p = buf;
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint64_t prime = UINT32_C(0x1000193);
#endif
    uint64_t hash = seed;

    while (len--) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 7) +
            (hash << 8) + (hash << 24);
#endif
    }
    return hash;
}

static inline uint64_t hash_fnv1a64_update(uint64_t v, uint8_t *buf, size_t len)
{
    uint8_t *p = buf;
#ifndef DVSTD_HASH_FNV1A_NOMUL
    const uint64_t prime = UINT64_C(0x100000001b3);
#endif
    uint64_t hash = v;

    while (len--) {
        hash ^= (uint64_t)*p++;
#ifndef DVSTD_HASH_FNV1A_NOMUL
        hash *= prime;
#else
        hash += (hash << 1) + (hash << 4) + (hash << 5) +
            (hash << 7) + (hash << 8) + (hash << 40);
#endif
    }
    return hash;
}

/*
 * MurmurHash 3 final mix with seed to handle 0.
 *
 * Width is number of bits of the value to return.
 * http://stackoverflow.com/a/12996028
 */
static inline uint32_t hash_bucket32(uint32_t v,  size_t width)
{
    uint32_t x = v + UINT32_C(0x2f693b52);

    x = ((x >> 16) ^ x) * UINT32_C(0x45d9f3b);
    x = ((x >> 16) ^ x) * UINT32_C(0x45d9f3b);
    x = ((x >> 16) ^ x);
    return x >> (32 - width);
}

/*
 * SplitMix64 - can be used to disperse fnv1a hash, to hash
 * an integer, or as a simple non-cryptographic prng.
 *
 * Width is number of bits of the value to return.
 * http://stackoverflow.com/a/12996028
 */
static inline uint64_t hash_bucket64(uint64_t v, size_t width)
{
    uint64_t x = v + UINT64_C(0x9e3779b97f4a7c15);

    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    x = x ^ (x >> 31);
    return x >> (64 - width);
}

/*
 * Faster, less random hash bucket compared to hash_bucket32, but works
 * for smaller integers.
 */
static inline uint32_t hash_mult32(uint32_t v, size_t width)
{
    /* Knuth's multiplicative hash. */
    return (v * UINT32_C(2654435761)) >> (32 - width);
}

#ifdef __cplusplus
}
#endif

#endif /* DVSTD_HASH_H */
