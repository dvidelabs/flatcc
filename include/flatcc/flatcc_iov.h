#ifndef FLATCC_IOV_H
#define FLATCC_IOV_H

#include <stdlib.h>

/*
 * The emitter receives one, or a few buffers at a time via
 * this type.  <sys/iov.h> compatible iovec structure used for
 * allocation and emitter interface.
 */
typedef struct flatcc_iovec flatcc_iovec_t;
struct flatcc_iovec {
    void *iov_base;
    size_t iov_len;
};

/*
 * The largest iovec vector the builder will issue. It will
 * always be a relatively small number.
 */
#define FLATCC_IOV_COUNT_MAX 8

#endif /* FLATCC_IOV_H */
