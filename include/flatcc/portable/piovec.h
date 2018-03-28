#ifndef PORTABLE_IOV_H
#define PORTABLE_IOV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

typedef struct iovec iovec_t;

struct iovec {
    void *iov_base;
    size_t iov_len;
};

static inline size_t iovlen(struct iovec *iov, int iovcnt)
{
    size_t len;
    int i;

    for (i = 0, len = 0; i < iovcnt; ++i) {
        len += iov[i].iov_len;
    }
    return len;
}

#ifdef __cplusplus
}
#endif

#endif /* PORTABLE_IOV_H */
