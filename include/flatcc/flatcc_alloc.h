#ifndef FLATCC_ALLOC_H
#define FLATCC_ALLOC_H

/*
 * Implements `aligned_alloc` and `aligned_free`.
 * Even with C11, this implements non-standard aligned_free needed for portable
 * aligned_alloc implementations.
 */
#ifndef FLATCC_NO_PALIGNED_ALLOC
#include "flatcc/portable/paligned_alloc.h"
#else
#ifndef aligned_free
#define aligned_free free
#endif
#endif

#ifndef FLATCC_ALLOC
#define FLATCC_ALLOC(n) malloc(n)
#endif

#ifndef FLATCC_FREE
#define FLATCC_FREE(p) free(p)
#endif

#ifndef FLATCC_REALLOC
#define FLATCC_REALLOC(p, n) realloc(p, n)
#endif

#ifndef FLATCC_ALIGNED_ALLOC
#define FLATCC_ALIGNED_ALLOC(a, n) aligned_alloc(a, n)
#endif

#ifndef FLATCC_ALIGNED_FREE
#define FLATCC_ALIGNED_FREE(p) aligned_free(p)
#endif

#endif /* FLATCC_ALLOC_H */
