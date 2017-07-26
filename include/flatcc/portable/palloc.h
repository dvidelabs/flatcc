#ifndef PALLOC_H
#define PALLOC_H

#define pMalloc(n) malloc(n)
#define pRealloc(p, n) realloc(p, n)
#define pFree(p) free(p)

#endif /* PALLOC_H */
