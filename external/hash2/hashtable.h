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

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Define HT_PRIVATE to make all name wrapping interface functions static
 * inline when including hash implementation directly in user code. This
 * can increase performance significantly (3x) on small hash tables with
 * fast hash functions because the compiler can better optimize static
 * functions. Some compiler optimizations will get the same speed
 * with external linkage (clang 4.2 -O4 but not -O3).
 *
 * Can also be used to simple hide the interface from global
 * linkage to avoid name clashes.
 */
#ifndef HT_PRIVATE
#define HT_PRIV
#else
#define HT_PRIV static inline
#endif


#define HT_EOK          0
#define HT_ENOMEM      -1
#define HT_ENOTUNIQ    -2
#define HT_ENOTFOUND   -3
#define HT_EINVAL      -4
#define HT_ENOTIMPL    -5
/* Implementation specific error codes at or below base */
#define HT_EIMPLBASE -100


/*
 * Generic hash table type. This makes it possible to use hash tables
 * in datastructures and header files that do not have access to
 * the specific hash table implementation. Call to init is optional
 * if the structure is zeroed.
 *
 * Offsets are only used with Robin Hood hashing to segment each chain.
 *
 * Keys and values are both stored in the same item pointer. There are
 * downsides to this over a key / value represention, but since we also
 * use less space we can afford lower the load factor and we can have a
 * more complex key representations. The smaller bucket size also helps
 * when ordering Robin Hood hash chains.
 */
typedef struct hashtable hashtable_t;

struct hashtable {
    void *table;
    unsigned char *offsets;
    size_t count;
    /* May be stored as a direct count, or log2. */
    size_t buckets;
};


/*
 * This macro defines the prototypes of the hash table that user code
 * needs for linkage.
 *
 * See also "hashtable_interface.h" which builds wrapper functions to a
 * generic hash table implementation so each specialization gets its own
 * set of named functions.
 *
 * The HT_ITEM is normally a pointer to and the hash table does not
 * store any signficant information internally.
 *
 * Some implemnetations define sentinel missing and deleted values while
 * other implementations store this information externally.
 */
#define declare_hashtable(HT_NAME, HT_KEY, HT_ITEM)                         \
                                                                            \
typedef hashtable_t HT_NAME##_t;                                            \
typedef HT_KEY HT_NAME##_key_t;                                             \
typedef HT_ITEM HT_NAME##_item_t;                                           \
                                                                            \
/*                                                                          \
 * Returns the missing value as defined for this type, or a zero            \
 * initialized item if missing values are not used.                         \
 */                                                                         \
HT_PRIV HT_ITEM HT_NAME##_missing();                                        \
                                                                            \
/*                                                                          \
 * Returns the deleted (tombstone) value as defined for this type,          \
 * or a zero initialized item if deleted values are not used.               \
 */                                                                         \
HT_PRIV HT_ITEM HT_NAME##_deleted();                                        \
                                                                            \
/*                                                                          \
 * Returns a human readable name for given hash table specific              \
 * error code. Always returns a valid string, even if undefined.            \
 */                                                                         \
HT_PRIV const char *HT_NAME##_strerror(int err);                            \
                                                                            \
/*                                                                          \
 *  Returns the hash value of a given key.                                  \
 */                                                                         \
HT_PRIV size_t HT_NAME##_hash(HT_KEY key);                                  \
                                                                            \
/*                                                                          \
 * Returns the key of a given hash table item.                              \
 */                                                                         \
HT_PRIV HT_KEY HT_NAME##_key(HT_ITEM item);                                 \
                                                                            \
/*                                                                          \
 * Returns 1 if the given keys match and 0 otherwise.                       \
 */                                                                         \
HT_PRIV int HT_NAME##_match(HT_KEY a, HT_KEY b);                            \
                                                                            \
/*                                                                          \
 * Prototype for user supplied callback when visiting all elements.         \
 * A call to `visit` will forward any non-zero return code and exit early.  \
 */                                                                         \
typedef int HT_NAME##_visitor_f(HT_ITEM item, void *context);               \
                                                                            \
/*                                                                          \
 * Allocates enough buckets to represent count elements without resizing.   \
 * The actual number of allocated buckets depends on the load factor        \
 * given as a macro argument in the implementation. The bucket number       \
 * rounds up to the nearest power of 2.                                     \
 *                                                                          \
 * `ht` should not be initialized beforehand, otherwise use resize.         \
 * Alternatively, it is also valid to zero initialize the table by          \
 * other means - this will postpone allocation until needed.                \
 *                                                                          \
 * The load factor (template argument) should be positive and at most       \
 * 100%, otherwise insertion and resize cannot succeed. The recommended     \
 * load factor is between 25% and 75%.                                      \
 *                                                                          \
 * Returns 0 on success, -1 on allocation failure or invalid load factor.   \
 */                                                                         \
HT_PRIV int HT_NAME##_init(HT_NAME##_t *ht, size_t count);                  \
                                                                            \
/*                                                                          \
 * Clears the allocated memory. Optionally takes a destructor               \
 * that will visit all items.                                               \
 * The table struct may be reused after being destroyed.                    \
 * May also be called on a zero initialised hash table.                     \
 *                                                                          \
 * Because the destructor is based on the `visit` implementation the        \
 * destructor will only be called until a call returns non-zero.            \
 * The table will be cleared regardless before returning the destructors    \
 * return code.                                                             \
 *                                                                          \
 * Can be called in place of clear for more control.                        \
 */                                                                         \
HT_PRIV int HT_NAME##_destroy(HT_NAME##_t *ht,                              \
        HT_NAME##_visitor_f *destructor, void *context);                    \
                                                                            \
/*                                                                          \
 * Clears the allocated memory, but does manage memory or state of any      \
 * stored items. It is a simpler version of destroy.                        \
 */                                                                         \
HT_PRIV void HT_NAME##_clear(HT_NAME##_t *ht);                              \
                                                                            \
/*                                                                          \
 * Resizes the hash table to hold at least `size` elements.  The actual     \
 * number of allocated buckets is a strictly larger power of two. If        \
 * `size` is smaller than the current number of elements, that number is    \
 * used instead of the given size. Thus, resize(ht, 0) may be used to       \
 * reduce the table size after a spike.  The function is called             \
 * automatically as elements are inserted, but shrinking the table          \
 * should be done manually.                                                 \
 *                                                                          \
 * If resizing to same size, table is still reallocated but will then       \
 * clean up old tombstones from excessive deletion.                         \
 *                                                                          \
 * Returns 0 on success, -1 on allocation failure.                          \
 */                                                                         \
HT_PRIV int HT_NAME##_resize(HT_NAME##_t *ht, size_t size);                 \
                                                                            \
/*                                                                          \
 * Inserts a new item if it does not match any existing                     \
 * item and otherwise returns HT_ENOTUNIQ. Returns 0 if                     \
 * an insertion took place. Returns HT_ENOMEM if out of                     \
 * memory.                                                                  \
 */                                                                         \
HT_PRIV int HT_NAME##_insert(HT_NAME##_t *ht,                               \
        HT_ITEM item);                                                      \
                                                                            \
/*                                                                          \
 * Inserts a new item if it does not match any existing item and            \
 * otherwise keeps the old item. The old item is placed in the result       \
 * if found and otherwise the result is left unmodified. If there are       \
 * multiple matches an arbitrary item is chosen. Returns 0 if an            \
 * insertion or update took place. Returns HT_ENOMEM if out of              \
 * memory. The result pointer may be null.                                  \
 */                                                                         \
HT_PRIV int HT_NAME##_insert_once(HT_NAME##_t *ht,                          \
        HT_ITEM item, HT_ITEM *result);                                     \
                                                                            \
/*                                                                          \
 * Inserts a new item if it does not match any existing item and            \
 * otherwise replaces it. The old item is placed in the result              \
 * if found and otherwise the result is left unmodified. If there are       \
 * multiple matches an arbitrary item is chosen. Returns 0 if an            \
 * insertion or update took place. Returns HT_ENOMEM if out of              \
 * memory. The result pointer may be null.                                  \
 */                                                                         \
HT_PRIV int HT_NAME##_upsert(HT_NAME##_t *ht,                               \
        HT_ITEM item, HT_ITEM *result);                                     \
                                                                            \
/*                                                                          \
 * Updates an existing item if present and returns the old                  \
 * item, otherwise returns HT_ENOTFOUND with the result                     \
 * unmodified. Returns HT_ENOMEM if out of memory.                          \
 * Returns 0 if an update took place. The result pointer                    \
 * may be null.                                                             \
 */                                                                         \
HT_PRIV int HT_NAME##_update(HT_NAME##_t *ht,                               \
        HT_ITEM item, HT_ITEM *result);                                     \
                                                                            \
/*                                                                          \
 * Inserts a new item even if one already exists. Returns                   \
 * HT_EINVAL if the hashtable does not support duplicates, even             \
 * if a duplicate was not found. Returns 0 if an insertion                  \
 * took place. Returns HT_ENOMEM if out of memory.                          \
 */                                                                         \
HT_PRIV int HT_NAME##_insert_multi(HT_NAME##_t *ht,                         \
        HT_ITEM item);                                                      \
                                                                            \
/* Returns 1 if key exists and 0 otherwise. */                              \
HT_PRIV int HT_NAME##_exists(HT_NAME##_t *ht,                               \
       HT_KEY key);                                                         \
                                                                            \
/*                                                                          \
 * Finds and returns a matching item, or HT_ENOTFOUND                       \
 * with the result unmodified. If there are multiple                        \
 * matches, an arbitrary item is returned. Returns                          \
 * 0 if found. The result pointer may be null.                              \
 */                                                                         \
HT_PRIV int HT_NAME##_find(HT_NAME##_t *ht,                                 \
        HT_KEY key, HT_ITEM *result);                                       \
                                                                            \
/*                                                                          \
 * Removes a single arbitrary matching item and returns                     \
 * the item, or returns HT_ENOTFOUND if no match was found.                 \
 */                                                                         \
HT_PRIV int HT_NAME##_delete(HT_NAME##_t *ht, HT_KEY key);                  \
                                                                            \
/*                                                                          \
 * Deletes an arbitrary matching item and stores the old                    \
 * item in the result if found. Otherwise returns                           \
 * HT_ENOTFOUND with the result unmodified.                                 \
 * Returns HT_ENOTIMPL if the operation is not supported.                   \
 * The result pointer may be null.                                          \
 */                                                                         \
HT_PRIV int HT_NAME##_extract(HT_NAME##_t *ht,                              \
        HT_KEY key, HT_ITEM *result);                                       \
                                                                            \
                                                                            \
/*                                                                          \
 * Calls a function for every item in the hash table. If the supplied       \
 * visitor function returns non-zero, visit will return that return         \
 * code immediately and abort the iteration. The optional destructor        \
 * is implemented using this mechanism.                                     \
 */                                                                         \
HT_PRIV int HT_NAME##_visit(HT_NAME##_t *ht,                                \
        HT_NAME##_visitor_f *visitor, void *context);                       \
                                                                            \
/*                                                                          \
 * Returns number of elements in the table. (Not necessarily the number of  \
 * unique keys.                                                             \
 */                                                                         \
HT_PRIV size_t HT_NAME##_size(HT_NAME##_t *ht);                             \
                                                                            \
/* Returns 1 if the hash table is empty and 0 otherwise in constant time. */\
HT_PRIV int HT_NAME##_is_empty(HT_NAME##_t *ht);                            \


#endif /* HASHTABLE_H */
