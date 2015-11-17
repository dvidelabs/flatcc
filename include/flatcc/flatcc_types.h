#ifndef FLATCC_TYPES_H
#define FLATCC_TYPES_H

#ifdef FLATCC_PORTABLE
#include "flatcc/flatcc_portable.h"
#endif

#include <stdlib.h>

#ifndef UINT8_MAX
#include <stdint.h>
#endif

/*
 * This should match generated type declaratios in
 * `flatbuffers_common_reader.h` (might have different name prefix).
 * Read only generated code does not depend on library code,
 * hence the duplication.
 */
#ifndef flatbuffers_types_defined
#define flatbuffers_types_defined

/*
 * uoffset_t and soffset_t must be same integer type, except for sign.
 * They can be (u)int16_t, (u)int32_t, or (u)int64_t.
 * The default is (u)int32_t.
 *
 * voffset_t is expected to be uint16_t, but can experimentally be
 * compiled from uint8_t up to uint32_t.
 *
 * ID_MAX is the largest value that can index a vtable. The table size
 * is given as voffset value. Each id represents a voffset value index
 * from 0 to max inclusive. Space is required for two header voffset
 * fields and the unaddressible highest index (due to the table size
 * representation). For 16-bit voffsets this yields a max of 2^15 - 4,
 * or (2^16 - 1) / 2 - 3.
 */
#define flatbuffers_uoffset_t_defined
#define flatbuffers_soffset_t_defined
#define flatbuffers_voffset_t_defined

#define FLATBUFFERS_UOFFSET_MAX UINT32_MAX
#define FLATBUFFERS_SOFFSET_MAX INT32_MAX
#define FLATBUFFERS_SOFFSET_MIN INT32_MIN
#define FLATBUFFERS_VOFFSET_MAX UINT16_MAX

#define FLATBUFFERS_UOFFSET_WIDTH 32
#define FLATBUFFERS_SOFFSET_WIDTH 32
#define FLATBUFFERS_VOFFSET_WIDTH 16

typedef uint32_t flatbuffers_uoffset_t;
typedef int32_t flatbuffers_soffset_t;
typedef uint16_t flatbuffers_voffset_t;

#define FLATBUFFERS_IDENTIFIER_SIZE 4

typedef char flatbuffers_fid_t[FLATBUFFERS_IDENTIFIER_SIZE];

#define FLATBUFFERS_ID_MAX (FLATBUFFERS_VOFFSET_MAX / sizeof(flatbuffers_voffset_t) - 3)
#define FLATBUFFERS_COUNT_MAX(elem_size) (FLATBUFFERS_UOFFSET_MAX/(elem_size))

#endif /* flatbuffers_types_defined */


/*
 * Here follows other types not present in generated code.
 */

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
 * This will always be a relatively small integer, but large enough to
 * support flatcc builder operations. The emitter will never receive
 * longer io vectors than this.
 */
#define FLATCC_IOV_COUNT_MAX 8


#endif /* FLATCC_TYPES_H */
