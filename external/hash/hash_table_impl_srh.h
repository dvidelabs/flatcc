/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Mikkel F. Jørgensen, dvide.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Sorted Robin Hashing.
 *
 * We cache the hash key and use it to decide the boundaries
 * of a hash chain. By inspiration by Paul Khuong we also keep hashes
 * sorted within a chain so binary or expential search is possible.
 *
 * For a given table size we fold the hash key by rotating K-1 bits M
 * positions, where K is the hash word size in bits, and M is the index
 * width in bits. The upper bit is set to 1. This transformation enables
 * global sorting and still use the highest quality bits in a typical
 * hash for bucket indexing. If entry belong to a bucket near the end is
 * wrapped and placed early in the table, the upper bit is cleared.  An
 * empty slot is assigned the maximum value. Upon resizing the table the
 * hash is unfolded and refolded with the new index width.
 */

/* We use the same define for all implementations */
#ifdef HASH_TABLE_IMPL
#error "cannot have multiple implementations in same compilation unit"
#endif
#define HASH_TABLE_IMPL
/* Sorted Robin Hood */
#define HT_SRH

#if defined(_MSC_VER)
#pragma warning(disable: 4127) /* conditional expression is constant */
#endif

#include <stdlib.h>
#include <memory.h>
#include <assert.h>

typedef struct ht_entry {
    size_t hash;
    ht_item_t item;
} ht_entry_t;

#define HT_HASH_MAX ((size_t)-1)
#define HT_HASH_WIDTH (sizeof(size_t) * 8)
#define HT_HASH_MSB ((size_t)1 << (HT_HASH_WIDTH - 1))

static inline size_t __ht_bucket_count(hash_table_t *ht)
{
    return (size_t)1 << ht->buckets;
}

#define HT_TRACE_ON
#include "ht_trace.h"

/* M is log2 of table size. */
static inline size_t __ht_fold_hash(size_t hash, int M)
{
    /* 
     * Ensure hash does not conflict with sentinel values.
     * MAX codes for empty slots, and MAX - 1 is never used
     * which makes it safe to search for any hash + 1 when
     * searching the next larger entry.
     */
    if (hash >= HT_HASH_MAX - 1) {
        /* Won't affect bucket index and will survive table resize. */
        hash = HT_HASH_MAX - HT_HASH_MSB / 2;
    }
    hash &= ~HT_HASH_MSB;
    return (hash << (HT_HASH_WIDTH - M - 1)) | (hash >> M) | HT_HASH_MSB;
}

static inline size_t __ht_unfold_hash(size_t fhash, int M)
{
    fhash &= ~HT_HASH_MSB;
    fhash = (fhash >> (HT_HASH_WIDTH - M - 1)) | (fhash << M);
    /*
     * This may differ from the original hash at MSB which we stripped
     * to make way for index wrap marker bit.
     */
    return fhash & ~HT_HASH_MSB;
}

/*
 * Increment bucket index. If it is that last index, it correctly flips
 * the MSB bit from 1 to 0. The lower bits are zeroed to produce the
 * smallest possible hash of this next bucket entry. This can be used
 * to search for the start of the next bucket.
 */
static inline size_t __ht_next_fold_hash(size_t hash, int M)
{
    size_t k = (1 << (HT_HASH_WIDTH - M - 1));

    return (hash + k) & ~(k - 1);
}

/* `hash` is given before fold. */
static inline size_t __ht_bucket_index(size_t hash, int M)
{
    return hash & ((1 << M) - 1);
}

static inline ssize_t __ht_bucket_dist(size_t index, size_t fhash, int M)
{
    /* Empty slots are always their right place. */
    if (fhash == HT_HASH_MAX) {
        return 0;
    }
    /*
     * The msb is a bit over the current index size M in bits.
     * If we overflow by wrapping, it is zero, and one otherwise
     * due to sort order, but here it works opposite: bit flipping
     * the bit we correctly get the wrapped distance.
     */
    return ((fhash ^ HT_HASH_MSB) >> (HT_HASH_WIDTH - M - 1)) - index;
}

/*
 * e0 is floor of search incl., e1 is start of search and e2 is
 * inclusive ceiling. The range is sorted but wraps so the the minimum
 * is not at the floor necessarily.  A match is the first entry with a
 * hash >= the search key. e2 and e0 should not be tested in advance
 * because it would hurt cache performance.
 *
 * We make certain assumptions on the array:
 * - all hash keys are sorted, except MAX_KEY may occur between other
 *   keys.
 * - there cannot be a MAX_KEY between e1 and some later key that is
 *   smaller than or equal to e1. If there were, the array would be
 *   shifted down to move the MAX_KEY until invariant holds again.
 * - the above works due to the Robin Hood invariant, and because we
 *   order the hash keys such that each hash bucket index populate the
 *   higher bits in the hash key.
 * - if there is a wrap, hash keys could violate sort order but not
 *   distance relation according to Robin Hood. To handle this
 *   efficiently, we reserve the most significant so it is cleared
 *   when storing a wrapped entry and set otherwise.
 * - the original hash key is rotated to match the given table size
 *   so the least significant bits still provide the bucket index
 *   because hash functions are generally better in the lower bits
 *   although this search is technically indifferent to this.
 * - A hash value equal to the maximum value has the msb bit cleared
 *   before rotating the hash value to ensure it lands in the same
 *   bucket and does not conflict with missing hash entries.
 *
 * The function cannot find empty slots by search for max hash key
 * because empty entries are not sorted.
 *
 */
static inline ht_entry_t *__ht_find_hash(ht_entry_t *e0, ht_entry_t *e1, ht_entry_t *e2, size_t hash)
{
    ht_entry_t *e;
    int i;

    assert(e1 <= e2);
    assert(e0 <= e1);
    assert(hash & HT_HASH_MSB);

    if (e1->hash >= hash) {
        return e1;
    }
    /* Because we statistically start at an offset. */
    e = e1;

    /*
     * Find the element after the first that is not strictly
     * smaller. This also finds the first empty slot if it is
     * hashed with largest possible key.
     */
    for (i = 0; i < 2; ++i) {
        if (e + 8 > e2) {
            if (e2->hash < hash) {
                e = e0;
                e2 = e1;
                /*
                 * Wrapped keys are smaller than other keys
                 * by clearing msb.
                 */
                hash &= ~HT_HASH_MSB;
                if (e->hash >= hash) {
                    return e;
                }
            } else {
                if (e == e2) {
                    return e;
                }
                break;
            }
        }
        if (e[1].hash >= hash) {
            return e + 1;
        }
        if (e[2].hash < hash) {
            if (e[4].hash < hash) {
                if (e[8].hash < hash) {
                    e += 8;
                    continue;
                } else {
                    if (e[6].hash < hash) {
                        if (e[7].hash < hash) {
                            return e + 8;
                        }
                        return e + 7;
                    } else {
                        if (e[5].hash < hash) {
                            return e + 6;
                        }
                        return e + 5;
                    }
                }
            } else {
                if (e[3].hash < hash) {
                    return e + 4;
                } else {
                    return e + 3;
                }
            }
        } else {
            return e + 2;
        }
    }
    /*
     * We now assume e2 is a <= tested upper bound and e is a < tested
     * lower bound, and e < e2. We now do a binary search and forget cache
     * performance because obviously the hash bucket has gone awol on
     * bad hash clustering. (We also end here when close the end of the
     * array to avoid optimized code going out of bounds.)
     */
#ifdef HT_BIN_SEARCH
    for (;;) {
        e1 = e + (e2 - e) / 2;
        if (e1->hash < hash) {
            e = e1;
            if (e == e1) {
                return e2;
            }
        } else {
            e2 = e1;
        }
    }
#else
    /* Exponential search. */
    i = 1;
    e1 = e + 1;
    for (;;) {
        if (e1->hash < hash) {
            e = e1;
            i *= 2;
            e1 += i;
            if (e1 < e2) {
                continue;
            }
        } else {
            e2 = e1;
        }
        i = 1;
        e1 = e + 1;
        if (e1 == e2) {
            return e2;
        }
    }
#endif
}

/*
 * Searches for matching key among identical folded hash values. If a
 * match is found the fhash value will match otherwise not. Unless the
 * table is full, an unsuccessful match returns the location of insertion
 * for such a value.
 */
static ht_entry_t *__ht_find_match(ht_entry_t *T, ht_entry_t *e1, ht_entry_t *e2,
        size_t fhash, const void *key, size_t len, int *found)
{
    ht_entry_t *e;

    e = __ht_find_hash(T, e1, e2, fhash);
    if (e >= e1) {
        while (e->hash == fhash) {
            if ((*found = ht_match(key, len, e->item))) {
                return e;
            }
            if (++e > e2) {
                e = T;
                break;
            }
        }
    }
    fhash = fhash & ~HT_HASH_MSB;
    while (e->hash == fhash) {
        if ((*found = ht_match(key, len, e->item))) {
            return e;
        }
        ++e;
    }
    *found = 0;
    return e;
}

/* Makes space at location e if not already available. */
static inline void __ht_alloc_entry(hash_table_t *ht, ht_entry_t *T, ht_entry_t *e, ht_entry_t *e2)
{
    ht_entry_t *u;

    assert(T != 0);
    assert(ht->table == T);
    assert(e >= T);
    assert(e <= e2);
    assert(e2 - T < __ht_bucket_count(ht));

    /*
     * Find first free entry. This is significantly more efficient
     * than linear probing on load factors close to 100%. We still
     * get linear move though. With 90% utilization we still only
     * have to move 5 buckets on average because the average length
     * is 9, and we insert into the middle of that on average. Only
     * for really bad hash functions do we benefit greatly and as
     * mentioned still stuffer linear move.
     */
    u = e;
    while (u->hash != HT_HASH_MAX) {
        if (u == e2) {
            u = T;
        } else {
            ++u;
        }
    }
    if (u < e) {
        assert(u >= T);
        memmove(T + 1, T, (u - T) * sizeof(*e));
        T->hash = e2->hash & ~HT_HASH_MSB;
        T->item = e2->item;
        memmove(e + 1, e, (e2 - e) * sizeof(*e));
    } else {
        assert(u <= e2);
        assert(e + 1 + (u - e) - T <= __ht_bucket_count(ht));
        memmove(e + 1, e, (u - e) * sizeof(*e));
    }
}

static int ht_init(hash_table_t *ht, size_t count)
{
    size_t buckets = 4;
    int M = 2;
    ht_entry_t *T;

    if ((HT_LOAD_FACTOR_FRAC) > 256 || (HT_LOAD_FACTOR_FRAC) < 1) {
        /*
         * 101% will never terminate insertion.
         * 0% will never terminate resize.
         */
        HT_PANIC("sorted robin hood hash table failed with impossible load factor");
        return -1;
    }
    while (count > buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        buckets = 1 << ++M;
    }
    /* Allocate extra sentinel entry for faster scan. */
    T = malloc(buckets * sizeof(ht_entry_t));
    if (T == 0) {
        return -1;
    }
    memset(T, -1, buckets * sizeof(ht_entry_t));
    ht->table = T;
    ht->offsets = 0;
    ht->buckets = M;
    ht->count = 0;
    return 0;
}

static int ht_resize(hash_table_t *ht, size_t count)
{
    hash_table_t ht2;
    ht_entry_t *T2, *e, *e2, *u, *u1, *u2;
    size_t hash, fhash;
    int M, M2;

    e = ht->table;
    if (!e) {
        return ht_init(ht, count);
    }
    e2 = e + __ht_bucket_count(ht) - 1;

    if (count < ht->count) {
        count = ht->count;
    }
    if (ht_init(&ht2, count)) {
        return -1;
    }
    M = (int)ht->buckets;
    M2 = (int)ht2.buckets;
    T2 = ht2.table;
    u2 = T2 + __ht_bucket_count(&ht2) - 1;
    while (e <= e2) {
        if (e->hash != HT_HASH_MAX) {
            hash = __ht_unfold_hash(e->hash, M);
            fhash = __ht_fold_hash(hash, M2);
            u1 = T2 + __ht_bucket_index(hash, M2);
            /* Safe because HT_HASH_MAX - 1 does not exist. */
            u = __ht_find_hash(T2, u1, u2, fhash + 1);
            if (u < u1) {
                fhash &= ~HT_HASH_MSB;
            }
            __ht_alloc_entry(&ht2, T2, u, u2);
            u->hash = fhash;
            u->item = e->item;
        }
        ++e;
    }
    ht2.count = ht->count;
    ht_clear(ht);
    *ht = ht2;
    return 0;
}

static ht_item_t ht_find(hash_table_t *ht, const void *key, size_t len)
{
    ht_entry_t *T, *e, *e1, *e2;
    int M = (int)ht->buckets;
    size_t hash = HT_HASH_FUNCTION(key, len);
    size_t fhash = __ht_fold_hash(hash, M);
    int found;

    T = ht->table;
    e1 = T + __ht_bucket_index(hash, M);
    e2 = T + __ht_bucket_count(ht) - 1;
    e = __ht_find_match(T, e1, e2, fhash, key, len, &found);
    if (found) {
        return e->item;
    }
    return 0;
}

static ht_item_t ht_insert(hash_table_t *ht, const void *key, size_t len, ht_item_t item, int mode)
{
    ht_entry_t *T, *e, *e1, *e2, *u;
    ht_item_t old_item;
    int M;
    size_t hash, fhash, buckets;
    int found;

    buckets = __ht_bucket_count(ht);
    if (ht->count >= buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        if (ht_resize(ht, ht->count * 2)) {
            HT_PANIC("hash table failed to allocate memory during resize");
            return HT_ALLOC_FAILED;
        }
        buckets = __ht_bucket_count(ht);
    }

    M = (int)ht->buckets;
    hash = HT_HASH_FUNCTION(key, len);
    fhash = __ht_fold_hash(hash, M);
    T = ht->table;
    e2 = T + buckets - 1;
    e1 = T + __ht_bucket_index(hash, M);
    if (mode == ht_multi) {
        /* Safe because HT_HASH_MAX - 1 does not exist. */
        e = __ht_find_hash(T, e1, e2, fhash + 1);
    } else {
        e = __ht_find_match(T, e1, e2, fhash, key, len, &found);
        if (found) {
            old_item = e->item;
            if (mode == ht_replace) {
                e->item = item;
            }
            return old_item;
        }
    }
    if (e < e1) {
        fhash &= ~HT_HASH_MSB;
    }
    __ht_alloc_entry(ht, T, e, e2);
    e->hash = fhash;
    e->item = item;
    ++ht->count;
    return 0;
}

static ht_item_t ht_remove(hash_table_t *ht, const void *key, size_t len)
{
    ht_entry_t *T, *e, *e1, *e2, *u;
    ht_item_t old_item;
    int M = (int)ht->buckets;
    size_t hash = HT_HASH_FUNCTION(key, len);
    size_t fhash = __ht_fold_hash(hash, M);
    ssize_t dist;
    int found;

    T = ht->table;
    e2 = T + __ht_bucket_count(ht);
    e1 = T + __ht_bucket_index(hash, M);
    e = __ht_find_match(T, e1, e2, fhash, key, len, &found);
    if (found) {
        return 0;
    }
    old_item = e->item;
    /*
     * Incrementally search for start of next bucket chain
     * until we find an empty bucket or a bucket whose chain
     * starts without offset. This also deals with wrapping.
     */
    u = e;
    while (__ht_bucket_dist(e - T, u->hash, M) > 0) {
        fhash = __ht_next_fold_hash(u->hash, M);
        u = __ht_find_hash(T, e, e2, fhash);
    }
    if (u < e) {
        memmove(e, e + 1, (e2 - e) * sizeof(*e));
        if (u == T) {
            u = e2;
        }  else {
            *e2 = *T;
            /* Undo wrap. */
            e2->hash |= HT_HASH_MSB;
            memmove(T, T + 1, (u - T) * sizeof(*e));
        }
    } else {
        memmove(e, e + 1, (u - e) * sizeof(*e));
    }
    --ht->count;
    u->hash = HT_HASH_MAX;
    return old_item;
}

static void ht_visit(hash_table_t *ht, ht_visitor_f *visitor, void *context)
{
    ht_entry_t *e, *e2;

    e = ht->table;
    e2 = e + __ht_bucket_count(ht) - 1;
    while (e <= e2) {
        if (e->hash != HT_HASH_MAX) {
            visitor(context, e->item);
        }
        ++e;
    }
}

static void ht_clear(hash_table_t *ht)
{
    if (ht->table) {
        free(ht->table);
    }
    memset(ht, 0, sizeof(*ht));
}
