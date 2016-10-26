#ifndef PALIGNED_ALLOC_H

#ifndef __cplusplus

/*
 * NOTE: MSVC in general has no aligned alloc function that is
 * compatible with free and it is not trivial to implement a version
 * there is. Therefore, to remain portable, end user code needs to
 * use `aligned_free` which is not part of C11 but defined in this header.
 *
 * The same issue may be present on some Unix systems not providing
 * posix_memalign, but we do not currently support this.
 *
 * For C11 compliant compilers and compilers with posix_memalign,
 * it is valid to use free instead of aligned_free with the above
 * caveats.
 */

#include <stdlib.h>
/* Test for GCC < 4.8.4 */
#if (!defined(__clang__) && \
    defined(__GNUC__) && (__GNUC__ < 4 || \
    (__GNUC__ == 4 && (__GNUC_MINOR__ < 8 || \
        (__GNUC_MINOR__ == 8 && \
            __GNUC_PATCHLEVEL__ < 4)))))
#undef PORTABLE_C11_ALIGNED_ALLOC_MISSING
#define PORTABLE_C11_ALIGNED_ALLOC_MISSING
#endif

#if defined(__IBMC__)
#undef PORTABLE_C11_ALIGNED_ALLOC_MISSING
#define PORTABLE_C11_ALIGNED_ALLOC_MISSING
#endif

#if !defined(PORTABLE_C11_STDALIGN_MISSING)
#if ((defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) && \
    !defined(PORTABLE_C11_ALIGNED_ALLOC_MISSING))

#include <stdalign.h>

#ifndef __aligned_alloc_is_defined
#define __aligned_alloc_is_defined 1
#endif

#endif
#endif

/* C11 or newer */
#if !defined(aligned_alloc) && !defined(__aligned_alloc_is_defined)

#if defined(_MSC_VER)

/* Aligned _aligned_malloc is not compatible with free. */
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define aligned_free(p) _aligned_free(p)
#define __aligned_alloc_is_defined 1
#define __aligned_free_is_defined 1

#elif !defined(PORTABLE_NO_POSIX_MEMALIGN)

#ifdef __clang__
#include "mm_malloc.h"
#endif

static inline void *__portable_aligned_alloc(size_t alignment, size_t size)
{
    int err;
    void *p = 0;
    err = posix_memalign(&p, alignment, size);
    if (err && p) {
        free(p);
        p = 0;
    }
    return p;
}

#define aligned_alloc(alignment, size) __portable_aligned_alloc(alignment, size)
#define aligned_free(p) free(p)
#define __aligned_alloc_is_defined 1
#define __aligned_free_is_defined 1

#else

static inline void *__portable_aligned_alloc(size_t alignment, size_t size)
{
    char *raw;
    void *buf;

    if (alignment < sizeof(void *)) {
        alignment = sizeof(void *);
    }
    raw = (char *)(((size_t)malloc(size + 2 * alignment - 1) + alignment - 1) & ~(alignment - 1));
    buf = raw + alignment;
    (void **)buf)[-1] = raw;
    return buf;
}

static inline void *__portable_aligned_free(void *p)
{
    free(((void **)p)[-1]);
}

#define aligned_alloc(alignment, size) __portable_aligned_alloc(alignment, size)
#define aligned_free(p) __portable_aligned_free(p)
#define __aligned_alloc_is_defined 1
#define __aligned_free_is_defined 1

#endif

#endif /* aligned_alloc */

#if !defined(aligned_free) && !defined(__aligned_free_is_defined)
#define aligned_free(p) free(p)
#define __aligned_free_is_defined 1
#endif

#endif /* __cplusplus */
#endif /* PALIGNED_ALLOC_H */
