#ifndef FLATCC_ALLOC_H
#define FLATCC_ALLOC_H

/*
 * These allocation abstractions are only for runtime libraries.
 * The flatcc compiler uses Posix allocation routines.
 *
 * This header makes it possible to use systems where malloc is not
 * valid to use. In this case the portable library will not help
 * because it implements Posix / C11 abstractions.
 */

#include <stdlib.h>

#ifndef FLATCC_ALLOC
#define FLATCC_ALLOC(n) malloc(n)
#endif

#ifndef FLATCC_FREE
#define FLATCC_FREE(p) free(p)
#endif

#ifndef FLATCC_REALLOC
#define FLATCC_REALLOC(p, n) realloc(p, n)
#endif

#ifndef FLATCC_CALLOC
#define FLATCC_CALLOC(nm, n) calloc(nm, n)
#endif

/*
 * Implements `aligned_alloc` and `aligned_free`.
 * Even with C11, this implements non-standard aligned_free needed for portable
 * aligned_alloc implementations.
 */
#ifndef FLATCC_USE_GENERIC_ALIGNED_ALLOC

#ifndef FLATCC_NO_PALIGNED_ALLOC
#include "flatcc/portable/paligned_alloc.h"
#else
#if !defined(__aligned_free_is_defined) || !__aligned_free_is_defined
#define aligned_free free
#endif
#endif

#else /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */

#ifndef FLATCC_ALIGNED_ALLOC
static inline void *__flatcc_aligned_alloc(size_t alignment, size_t size)
{
    char *raw;
    void *buf;
    size_t total_size = (size + alignment - 1 + sizeof(void *));

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    raw = (char *)(size_t)FLATCC_ALLOC(total_size);
    buf = raw + alignment - 1 + sizeof(void *);
    buf = (void *)(((size_t)buf) & ~(alignment - 1));
    ((void **)buf)[-1] = raw;
    return buf;
}
#define FLATCC_ALIGNED_ALLOC(alignment, size) __flatcc_aligned_alloc(alignment, size)
#endif /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */

#ifndef FLATCC_ALIGNED_FREE
static inline void __flatcc_aligned_free(void *p)
{
    char *raw = ((void **)p)[-1];

    FLATCC_FREE(raw);
}
#define FLATCC_ALIGNED(p) __flatcc_aligned_free(p)
#endif

#endif /* FLATCC_USE_GENERIC_ALIGNED_ALLOC */

#ifndef FLATCC_ALIGNED_ALLOC
#define FLATCC_ALIGNED_ALLOC(a, n) aligned_alloc(a, n)
#endif

#ifndef FLATCC_ALIGNED_FREE
#define FLATCC_ALIGNED_FREE(p) aligned_free(p)
#endif

#endif /* FLATCC_ALLOC_H */
