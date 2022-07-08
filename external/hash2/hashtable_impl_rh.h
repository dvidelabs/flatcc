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

/* We use the same define for all implementations */
#ifdef HASHTABLE_IMPL
#error "cannot have multiple implementations in same compilation unit"
#endif
#define HASHTABLE_IMPL
/* Robin Hood (with offset table) */
#define HT_RH

#if defined(_MSC_VER)
#pragma warning(disable: 4127) /* conditional expression is constant */
#endif

#include <stdlib.h>
#include <string.h>

#include <dvstd/trace.h>
#include <assert.h>

/*
 * A variation of Robin Hashing:
 *
 * We do not calcute distance from buckets, nor do we cache hash keys.
 * Instead we maintain an 8-bit offset that points to where the first
 * entry of a bucket is stored + 1 or 0 for missing entry. If the offset
 * overflows the table is resized. In Robin Hood hashing all entries
 * conceptually chained to the same bucket are stored immediately after
 * each other in order of insertion. The offset of the next bucket is
 * naturally the end of the previous bucket, off by one. This breaks
 * down when the bucket offset is 0 and the bucket is empty because it
 * suggests there is an element. We cannot distinguish between a single
 * used and unused entry, except by looking at the content or otherwise
 * tag the information on. This is not a problem, just a special case to
 * deal with.
 *
 * The offsets are stored separately which might lead to more cache line
 * traffic, but the alternative is not very elegant - either wasting
 * space or trying to pack offsets on a per cache line basis. We only
 * need 8 bits for offsets (the expected maximum is about 6). During
 * insertion, offsets are incremented on all affected buckets, and
 * likewise decremented on remove.
 *
 * The approach bears some resemblance to hopscotch hashing which
 * uses local offsets for chaining, but we prefer the simpler Robin
 * Hood approach.
 *
 * The key benefit of Robin Hood hashing over Open Addressing is that
 * sentinel values for missing and deleted entries, more robust handling
 * of many deleted items where open addressing needs to rebalance,
 * and a more dense load factor. However, a dense load factor is not
 * always that beneficial when onl a single pointer occupie each hash
 * bucket. So open addressing with cache friendly linear probing is
 * still a good choice for many use cases.
 *
 * The separate offset table has a distinct advantage in fast failing
 * a missing key, but for large hash tables they incur an an extra
 * cache line load which makes it slower than open addressing when
 * the drawbacks of open addressing are acceptable (sentinel values
 * for missing and deleted items, and a lower load factor).
 *
 * The Robin hood hash table works by storing all collisions together
 * in a so called probe that either starts at the hash bucket directly
 * or at some relatively short offset. This offset is stored in an
 * offset table. The probe length is the difference between two offsets
 * + 1. This is modified slightly (which complicates many things) by
 * using offset 0 to code for a vacant bucket and store offsets + 1.
 *
 * When inserting, there is either a vacant bucket at the end, or
 * one or more probes occupy the following space. One item in each
 * probe is kicked down to the next probe until an item lands in an
 * empty bucket and offsets adjusted accordingly. Deletion is in
 * principle the reverse: gaps are kicked down until it lands where
 * the next bucket is vacant or unaffected.
 *
 * The classic Robin Hood algorithm need som heuristics to find the
 * probe start, but since offsets and not lengths are stored, we
 * can find the exact start and end of each probe without guessing.
 *
 * The table allows for multiple identical keys, and this is fast
 * because there is no need to search for matching key in the probe.
 * Multiple keys are NOT LIFO although it could be implemented. LIFO
 * would complicate insertion and resizing.
 *
 * Traditional Robin Hood Hashing actually permits a chain to become
 * very long. We do not permit this, in line with hopscotch hashing.
 * This is a drawback from a security perspective because worst case
 * this can trigger resizing ad infinitum iff the hash function can
 * be hacked or massive duplicate key insertion can be triggered. By
 * used the provided hash functions and seeding them randomly at
 * startup, and avoiding the multi key feature, it is very unlikely to
 * be a problem with what is known about hash table attacks so far.
 *
 * Values and keys do not have to be stored directly. Items can be
 * pointers from which a key can can be derived, but they can also be
 * structs with keys and values, or just single value that is both key
 * and data.
 *
 * A typical hash table has: key pointer or key value, value pointer
 * or value, a cached hash key or bitmap (for Robin Hood or Hopscotch)
 * which on 64 bit platforms easily amounts to 20 bytes or more per
 * bucket. We use 9 bytes on 64 bit platforms and 5 bytes on 32 bit.
 * This gets us down to a max load of 0.5 and on average about 0.37.
 * This should make it very likely that the first bucket inspected is
 * a direct hit negating the benefit of caching hash keys. In addition,
 * when it is not a direct hit, we get pointers loaded in a cache line
 * to inspect, all known to have the same hash key.
 */


int ht_init(hashtable_t *ht, size_t count)
{
    size_t buckets = 4;

    if ((HT_LOAD_FACTOR_FRAC) > 256 || (HT_LOAD_FACTOR_FRAC) < 1) {
        /*
         * 101% will never terminate insertion.
         * 0% will never terminate resize.
         */
        HT_PANIC("robin hood hash table failed with impossible load factor", HT_EINVAL);
        return -1;
    }
    while (count > buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        buckets *= 2;
    }
    ht->table = HT_ALLOC(buckets * sizeof(ht_item_t));
    if (ht->table == 0) {
        return -1;
    }
    ht->offsets = HT_CALLOC(buckets, sizeof(char));
    if (ht->offsets == 0) {
        HT_FREE(ht->table);
        ht->table = 0;
        return -1;
    }
    ht->buckets = buckets;
    ht->count = 0;
    return HT_EOK;
}

int ht_resize(hashtable_t *ht, size_t count)
{
    size_t i;
    hashtable_t ht2;
    ht_item_t *T = ht->table;

    if (count < ht->count) {
        count = ht->count;
    }
    if (ht_init(&ht2, count)) {
        return -1;
    }
    for (i = 0; i < ht->buckets; ++i) {
        if (ht->offsets[i]) {
            ht_insert(&ht2, T[i], 0, HT_MULTI);
        }
    }
    ht_clear(ht);
    *ht = ht2;
    return HT_EOK;
}

static size_t ht_size(hashtable_t *ht)
{
    return ht->count;
}

static int ht_is_empty(hashtable_t *ht)
{
    return ht->count == 0;
}

/*
 * The k'th probe is empty and starts at and ends before `bucket(k)` if
 * offset(k) is zero. Otherwise the probe starts at `bucket(k) +
 * offset(k) - 1` and ends at either before `bucket(k + 1)` if offset(k +
 * 1) is zero and otherwise before `bucket(k + 1) + offset(k + 1) - 1`.
 * A probe can have zero length even if the offset is non-zero. This
 * means that the bucket is in use by a preceding probe.
 *
 * Insertion first searches for matches in the k'th probe, then
 * creates space for insertion by kicking out a value at the start
 * of each following probe with non-zero length, until an empty
 * bucket is found (as opposed to a zero length probe). All offsets
 * after k must incremented by, also for the empty probes.
 *
 * If the last empty bucket is not the first bucket, the offset of the
 * bucket must change from 0 to 2 to make it a zero length probe.
 * Intuitively it is incremented once to make the bucket none-empty, and
 * once more to shift it's location like all other affected offsets.
 *
 * Note that the last probe before a probe with offset 1 or 0 will
 * always have offset 2 and zero length, unless the initial probe length
 * is 1 with offset 1, or vacant.
 */

int ht_insert(hashtable_t *ht, ht_item_t item, ht_item_t *result, int mode)
{
    ht_item_t *T;
    ht_item_t kick;
    ht_key_t key;
    size_t N, i, j, k;
    uint8_t offset;
    int overflow = 0;

    key = ht_key(item);
    if (ht->count >= ht->buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        if (ht_resize(ht, ht->count * 2)) {
            return HT_PANIC("robin hood hash table failed to allocate memory during resize", HT_EINVAL);
        }
    }
    T = ht->table;
    N = ht->buckets - 1;
    k = ht_hash(key) & N; /* first probe index */
    offset = ht->offsets[k]; /* first probe offset */
    if (offset == 0) { /* fast path, first probe bucket is vacant */
        if (mode == HT_UPDATE) return HT_ENOTFOUND;
        ++ht->count;
        T[k] = item;
        ht->offsets[k] = 1;
        return HT_EOK;
    }
    i = (k + offset - 1) & N; /* first probe start */
    k = (k + 1) & N; /* second probe */
    offset = ht->offsets[k]; /* second probe offset */
    j = (k + offset - 1 + (offset == 0)) & N; /* second probe start */
    if (mode != HT_MULTI) {
        while (i != j) {
            if (ht_match(key, ht_key(T[i]))) {
                if (result) *result = T[i];
                if (mode != HT_UPDATE && mode != HT_UPSERT) return HT_ENOTUNIQ;
                T[i] = item;
                return HT_EOK;
            }
            i = (i + 1) & N;
        }
        if (mode == HT_UPDATE) return HT_ENOTFOUND;
    }
    /* j is first bucket to kick */
    ++ht->count;
    if (offset == 0) {
        ht->offsets[k] = 2;
        T[k] = item;
        return HT_EOK;
    }
    do {
        overflow |= !(ht->offsets[k] = offset + 1);
        k = (k + 1) & N;
        offset = ht->offsets[k];
        i = j;
        j = (k + offset - 1 + (offset == 0)) & N;
        /* i == j would result in incorrect updates */
        if (i != j) {
            kick = T[i];
            T[i] = item;
            item = kick;
        }
    } while (offset);
    ht->offsets[k] = 2;
    T[k] = item;
    if (overflow) {
        trace("resize on robin hood probe length overflow");
        /* At least one offset overflowed so the table must be resized. */
        if (ht->count * 10 < ht->buckets) {
            return HT_PANIC("FATAL: hash table resize on low utilization would explode\n"\
                   "  possible collision DoS or bad hash function", HT_EINVAL);
        }
        if (ht_resize(ht, ht->count * 2)) {
            /* This renders the hash table in an inconsistent state due to kicks already performed. */
            return HT_PANIC("FATAL: hash table resize failed and left hash table inconsistent", HT_ENOMEM);\
        }
        trace("end resize on robin hood probe length overflow");
    }
    return HT_EOK;
}

int ht_find(hashtable_t *ht, ht_key_t key, ht_item_t *result)
{
    ht_item_t *T = ht->table;
    size_t N, i, j, k;
    uint8_t offset;

    if (T == 0) return HT_ENOTFOUND;
    N = ht->buckets - 1;
    k = ht_hash(key) & N;
    offset = ht->offsets[k];
    if (offset == 0) return HT_ENOTFOUND;
    i = (k + offset - 1) & N;
    k = (k + 1) & N;
    offset = ht->offsets[k];
    j = (k + offset - 1 + (offset == 0)) & N;
    while (i != j) {
        if (ht_match(key, ht_key(T[i]))) {
            if (result) *result = T[i];
            return HT_EOK;
        }
        i = (i + 1) & N;
    }
    return HT_ENOTFOUND;
}

/* Trying to speed up slow version */
int ht_delete(hashtable_t *ht, ht_key_t key, ht_item_t *result)
{
    ht_item_t *T = ht->table;
    size_t N, i, j, k, offset;

    if (T == 0) {
        return HT_ENOTFOUND;
    }
    N = ht->buckets - 1;
    k = ht_hash(key) & N;
    offset = ht->offsets[k];
    if (offset == 0) return HT_ENOTFOUND;
    i = (k + offset - 1) & N;
    k = (k + 1) & N;
    offset = ht->offsets[k];
    j = (k + offset - 1 + (offset == 0)) & N;
    for (;;) {
        if (i == j) return HT_ENOTFOUND;
        if (ht_match(key, ht_key(T[i]))) break;
        i = (i + 1) & N;
    }
    --ht->count;
    if (result) *result = T[i];
    j = (j - 1) & N;
    /* This unnecessary test actually provides a 2x speedup in some cases. */
    if (i != j) T[i] = T[j];
    while (offset > 1) {
        ht->offsets[k]--;
        k = (k + 1) & N;
        offset = ht->offsets[k];
        i = j;
        j = (k + offset - 1 - 1 + (offset == 0)) & N;
        if (i != j) T[i] = T[j];
    }
    ht->offsets[(k - 1) & N] = 0;
    return HT_EOK;
}

int ht_visit(hashtable_t *ht, ht_visitor_f *visitor, void *context)
{
    int ret;
    size_t i;
    ht_item_t *T = ht->table;

    for (i = 0; i < ht->buckets; ++i) {
        if (ht->offsets[i]) {
            if ((ret = visitor(T[i], context))) {
                return ret;
            }
        }
    }
    return 0;
}

void ht_clear(hashtable_t *ht)
{
    if (ht->table) {
        HT_FREE(ht->table);
    }
    if (ht->offsets) {
        HT_FREE(ht->offsets);
    }
    memset(ht, 0, sizeof(*ht));
}

static const char *ht_strerror(int err)
{
    return ht_strerror_default(err);
}

/* Not needed for this implementation, return 0 initialized item. */
static inline ht_item_t ht_missing()
{
    static ht_item_t x;
    return x;
}

/* Not needed for this implementation, return 0 initialized item. */
static inline ht_item_t ht_deleted()
{
    static ht_item_t x;
    return x;
}
