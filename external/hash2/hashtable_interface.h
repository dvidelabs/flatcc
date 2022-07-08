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

#ifndef HASHTABLE_INTERFACE
#define HASHTABLE_INTERFACE 1

#ifndef HT_ALLOC
#include <dvstd/alloc.h>
#define HT_ALLOC DVSTD_ALLOC
#define HT_REALLOC DVSTD_REALLOC
#define HT_CALLOC DVSTD_CALLOC
#define HT_FREE DVSTD_FREE
#endif

#ifndef HT_PANIC
#include <dvstd/panic.h>
#define HT_PANIC(s, ret) DVSTD_PANIC(s, ret)
#endif

/* Predefined hash functions to simplify implementing `ht_hash`. */
#include <dvstd/hash/hash.h>

/* insert mode (one of) */
#define HT_INSERT 0 /* default */
#define HT_UPDATE 1
#define HT_UPSERT 2
#define HT_MULTI  4


static const char *ht_strerror_default(int error)
{
    switch (error) {
    case HT_EOK: return "ok";
    case HT_ENOMEM: return "out of memory";
    case HT_ENOTUNIQ: return "not unique";
    case HT_ENOTFOUND: return "not found";
    case HT_EINVAL: return "invalid";
    case HT_ENOTIMPL: return "not implemented";
    default: return error <= HT_EIMPLBASE ?
             "implementation specific error" : "undefined error";
    }
}


/*
 * Instead of caching clever data per hash item, such as caching
 * the hashed key, make each item smaller and reduce the load
 * factor for the same memory consumption, or at least measure first.
 */
#ifndef HT_LOAD_FACTOR
#define HT_LOAD_FACTOR 0.7
#endif

#define HT_LOAD_FACTOR_FRAC ((int)((float)(HT_LOAD_FACTOR)*256))

#define HT_OFFSET_MISSING 0
#define HT_OFFSET_DELETED 1

/*
 * If using integer items, e.g. INT_MAX or INT_MIN may be more appropriate.
 * For pointers a sentinel pointer to a valid object could also be used
 * but defaults to the null pointer.
 */
#ifndef HT_MISSING
#define HT_MISSING ((ht_item_t)0)
#else
#define HT_MISSING_REDEFINED 1
#endif

/* Must be defined if the item is a struct. */
#ifndef HT_IS_MISSING
#define HT_IS_MISSING(item) ((item) == HT_MISSING)
#endif

/*
 * Only needed for some hash tables. Must be defined if the item is a struct.
 * and must be different from both valid and missing items.
 */
#ifndef HT_DELETED
/* '1' is not always defined, but often works like a null pointer. */
#define HT_DELETED ((ht_item_t)1)
#endif

#ifndef HT_IS_DELETED
#define HT_IS_DELETED(item) ((item) == HT_DELETED)
#endif


#define define_hashtable(HT_NAME)                                           \
                                                                            \
typedef HT_NAME##_item_t ht_item_t;                                         \
typedef HT_NAME##_key_t ht_key_t;                                           \
typedef HT_NAME##_visitor_f ht_visitor_f;                                   \
                                                                            \
/* User supplied. */                                                        \
static inline size_t ht_hash(ht_key_t key);                                 \
static inline int ht_match(ht_key_t a, ht_key_t b);                         \
static inline ht_key_t ht_key(ht_item_t item);                              \
                                                                            \
/* Implementation supplied. */                                              \
static int ht_insert(hashtable_t *ht, ht_item_t item,                       \
        ht_item_t *result, int mode);                                       \
static int ht_find(hashtable_t *ht, ht_key_t key, ht_item_t *result);       \
static int ht_delete(hashtable_t *ht, ht_key_t key, ht_item_t *result);     \
static int ht_init(hashtable_t *ht, size_t count);                          \
static int ht_resize(hashtable_t *ht, size_t count);                        \
static void ht_clear(hashtable_t *ht);                                      \
static size_t ht_size(hashtable_t *ht);                                     \
static int ht_is_empty(hashtable_t *ht);                                    \
static int ht_visit(hashtable_t *ht,                                        \
        ht_visitor_f *visitor, void *context);                              \
static const char *ht_strerror(int error);                                  \
static ht_item_t ht_missing();                                              \
static ht_item_t ht_deleted();                                              \
                                                                            \
HT_PRIV ht_item_t HT_NAME##_missing()                                       \
{                                                                           \
    return ht_missing();                                                    \
}                                                                           \
                                                                            \
HT_PRIV ht_item_t HT_NAME##_deleted()                                       \
{                                                                           \
    return ht_deleted();                                                    \
}                                                                           \
                                                                            \
HT_PRIV const char *HT_NAME##_strerror(int error)                           \
{                                                                           \
    return ht_strerror(error);                                              \
}                                                                           \
                                                                            \
HT_PRIV size_t HT_NAME##_hash(ht_key_t key)                                 \
{                                                                           \
    return ht_hash(key);                                                    \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_match(ht_key_t a, ht_key_t b)                         \
{                                                                           \
    return ht_match(a, b);                                                  \
}                                                                           \
                                                                            \
HT_PRIV ht_key_t HT_NAME##_key(ht_item_t item)                              \
{                                                                           \
    return ht_key(item);                                                    \
}                                                                           \
                                                                            \
HT_PRIV void HT_NAME##_clear(HT_NAME##_t *ht)                               \
{                                                                           \
    ht_clear(ht);                                                           \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_destroy(HT_NAME##_t *ht,                              \
        HT_NAME##_visitor_f *destructor, void *context)                     \
{                                                                           \
    int ret = 0;                                                            \
                                                                            \
    if (destructor) {                                                       \
        ret = ht_visit(ht, destructor, context);                            \
    }                                                                       \
    ht_clear(ht);                                                           \
    return ret;                                                             \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_init(HT_NAME##_t *ht, size_t count)                   \
{                                                                           \
    return ht_init(ht, count);                                              \
}                                                                           \
                                                                            \
HT_PRIV size_t HT_NAME##_size(HT_NAME##_t *ht)                              \
{                                                                           \
    return ht_size(ht);                                                     \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_is_empty(HT_NAME##_t *ht)                             \
{                                                                           \
    return ht_is_empty(ht);                                                 \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_resize(HT_NAME##_t *ht, size_t count)                 \
{                                                                           \
    return ht_resize(ht, count);                                            \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_insert(HT_NAME##_t *ht,                               \
        ht_item_t item)                                                     \
{                                                                           \
    return ht_insert(ht, item, 0, 0);                                       \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_insert_once(HT_NAME##_t *ht,                          \
        ht_item_t item, ht_item_t *result)                                  \
{                                                                           \
    return ht_insert(ht, item, result, 0);                                  \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_update(HT_NAME##_t *ht,                               \
        ht_item_t item, ht_item_t *result)                                  \
{                                                                           \
    return ht_insert(ht, item, result, HT_UPDATE);                          \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_upsert(HT_NAME##_t *ht,                               \
        ht_item_t item, ht_item_t *result)                                  \
{                                                                           \
    return ht_insert(ht, item, result, HT_UPSERT);                          \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_insert_multi(HT_NAME##_t *ht,                         \
        ht_item_t item)                                                     \
{                                                                           \
    return ht_insert(ht, item, 0, HT_MULTI);                                \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_exists(HT_NAME##_t *ht,                               \
        ht_key_t key)                                                       \
{                                                                           \
    return HT_EOK == ht_find(ht, key, 0);                                   \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_find(HT_NAME##_t *ht,                                 \
        ht_key_t key, ht_item_t *result)                                    \
{                                                                           \
    return ht_find(ht, key, result);                                        \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_delete(HT_NAME##_t *ht, ht_key_t key)                 \
{                                                                           \
    return ht_delete(ht, key, 0);                                           \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_extract(HT_NAME##_t *ht,                              \
        ht_key_t key, ht_item_t *result)                                    \
{                                                                           \
    return ht_delete(ht, key, result);                                      \
}                                                                           \
                                                                            \
HT_PRIV int HT_NAME##_visit(HT_NAME##_t *ht,                                \
        HT_NAME##_visitor_f *visitor, void *context)                        \
{                                                                           \
    return ht_visit(ht, visitor, context);                                  \
}                                                                           \

#endif /* HASHTABLE_INTERFACE */
