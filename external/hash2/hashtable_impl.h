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
 * This file implements a generic hash interface such that different
 * instances have the same name, but hidden from each other.
 * The interface maps the local names to a public specific type.
 *
 * This implementations implements a hash table with linear or quadratic
 * probing.
 */

#ifdef HASHTABLE_IMPL
#error "cannot have multiple implementations in same compilation unit"
#endif
#define HASHTABLE_IMPL
/* Open Addressing */
#define HT_OA

#if defined(_MSC_VER)
#pragma warning(disable: 4127) /* conditional expression is constant */
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifndef HT_PROBE
#ifdef HT_PROBE_QUADRATIC
#define HT_PROBE(k, i, N) ((k + (i + i * i) / 2) & N)
#else
#define HT_PROBE(k, i, N) ((k + i) & N)
#endif
#endif

#define HT_IS_VALID(item) (!HT_IS_MISSING(item) && !HT_IS_DELETED(item))

static inline ht_item_t ht_missing()
{
    ht_item_t x = HT_MISSING;
    return x;
}

static inline ht_item_t ht_deleted()
{
    ht_item_t x = HT_DELETED;
    return x;
}

#ifdef HT_MISSING_REDEFINED
static void ht_clear_items(ht_item_t *T, size_t count)
{
    size_t i;

    for (i = 0; i < count; ++i) {
        T[i] = ht_missing();
    }
}
#else
static inline void ht_clear_items(ht_item_t *T, size_t count)
{
    memset(T, 0, sizeof(T[0]) * count);
}
#endif

static int ht_init(hashtable_t *ht, size_t count)
{
    size_t buckets = 4;

    if (HT_IS_MISSING(ht_deleted())) {
        /* HT_DELETED is used to mark deleted entries. */
        return HT_PANIC("hash table 'missing' and 'deleted' values must be different in this implementation", HT_EINVAL);
    }

    if ((HT_LOAD_FACTOR_FRAC) > 256 || (HT_LOAD_FACTOR_FRAC) < 1) {
        /*
         * 100% is bad but still the users choice.
         * 101% will never terminate insertion.
         */
        return HT_PANIC("hash table failed with impossible load factor", HT_EINVAL);
    }
    while (count > buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        buckets *= 2;
    }
    ht->table = HT_ALLOC(buckets * sizeof(ht_item_t));
    if (ht->table == 0) {
        return -1;
    }
    ht->offsets = 0;
    ht->buckets = buckets;
    ht->count = 0;
    ht_clear_items(ht->table, buckets);
    return HT_EOK;
}

static int ht_resize(hashtable_t *ht, size_t count)
{
    size_t i;
    hashtable_t ht2;
    ht_item_t *T = ht->table;
    ht_item_t item;

    if (count < ht->count) {
        count = ht->count;
    }
    if (ht_init(&ht2, count)) {
        return -1;
    }
    for (i = 0; i < ht->buckets; ++i) {
        if (HT_IS_VALID(T[i])) {
            item = T[i];
            ht_insert(&ht2, item, 0, HT_MULTI);
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

static int ht_insert(hashtable_t *ht, ht_item_t item, ht_item_t *result, int mode)
{
    ht_item_t *T;
    size_t N, i, j, k;
    ht_item_t *vacant = 0;
    ht_key_t key;
    int probes = 0;

    assert(HT_IS_VALID(item));

    key = ht_key(item);
    if (ht->count >= ht->buckets * (HT_LOAD_FACTOR_FRAC) / 256) {
        if (ht_resize(ht, ht->count * 2)) {
            return HT_PANIC("hash table failed to allocate memory during resize", HT_ENOMEM);
        }
    }
    T = ht->table;
    N = ht->buckets - 1;
    k = ht_hash(key);
    i = 0;
    j = HT_PROBE(k, i, N);
    if (mode == HT_MULTI) {
        ++ht->count;
        while (HT_IS_VALID(T[j])) {
            ++i;
            j = HT_PROBE(k, i, N);
        }
        T[j] = item;
        return HT_EOK;
    }
    while (!HT_IS_MISSING(T[j])) {
        if (HT_IS_DELETED(T[j])) {
            ++probes;
            if (vacant == 0) {
                /*
                 * If a tombstone was found, use the first available,
                 * but continue search for possible match.
                 */
                vacant = &T[j];
            }
        } else if (ht_match(key, ht_key(T[j]))) {
            if (result) *result = T[j];
            if (mode != HT_UPDATE && mode != HT_UPSERT) {
                return HT_ENOTUNIQ;
            }
            T[j] = item;
            return HT_EOK;
        }
        ++i;
        j = HT_PROBE(k, i, N);
    }
    if (mode == HT_UPDATE) {
        return HT_ENOTFOUND;
    }
    if (vacant == 0) {
        vacant = &T[j];
    }
    ++ht->count;
    *vacant = item;
    if (probes > 32) {
        if (ht_resize(ht, ht->count)) {
            return HT_PANIC("hash table failed to allocate memory during resize", HT_ENOMEM);
        }
    }
    return HT_EOK;
}

static int ht_find(hashtable_t *ht, ht_key_t key, ht_item_t *result)
{
    ht_item_t *T = ht->table;
    size_t N, i, j, k;

    if (T == 0) {
        return HT_ENOTFOUND;
    }
    N = ht->buckets - 1;
    k = ht_hash(key);
    i = 0;
    j = HT_PROBE(k, i, N);
    while (!HT_IS_MISSING(T[j])) {
        if (!(HT_IS_DELETED(T[j])) &&
                ht_match(key, ht_key(T[j]))) {
            if (result) *result = T[j];
            return HT_EOK;
        }
        ++i;
        j = HT_PROBE(k, i, N);
    }
    return HT_ENOTFOUND;
}

static int ht_delete(hashtable_t *ht, ht_key_t key, ht_item_t *result)
{
    ht_item_t *T = ht->table;
    size_t N, i, j, k;

    if (T == 0) {
        return HT_ENOTFOUND;
    }
    N = ht->buckets - 1;
    k = ht_hash(key);
    i = 0;
    j = HT_PROBE(k, i, N);
    while (!HT_IS_MISSING(T[j])) {
        if (!HT_IS_DELETED(T[j]) &&
                ht_match(key, ht_key(T[j]))) {
            if (result) *result = T[j];
            T[j] = ht_deleted();
            --ht->count;
            return HT_EOK;
        }
        ++i;
        j = HT_PROBE(k, i, N);
    }
    return HT_ENOTFOUND;
}

static int ht_visit(hashtable_t *ht, ht_visitor_f *visitor, void *context)
{
    int ret;
    size_t i;
    ht_item_t *T = ht->table;

    for (i = 0; i < ht->buckets; ++i) {
        if (HT_IS_VALID(T[i])) {
            if ((ret = visitor(T[i], context))) {
                return ret;
            }
        }
    }
    return 0;
}

static void ht_clear(hashtable_t *ht)
{
    if (ht->table) {
        HT_FREE(ht->table);
    }
    memset(ht, 0, sizeof(*ht));
}

static const char *ht_strerror(int err)
{
    return ht_strerror_default(err);
}
