#ifndef PUNLIGNED_H
#define PUNLIGNED_H

#ifndef PORTABLE_UNALIGNED_ACCESS

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#define PORTABLE_UNALIGNED_ACCESS 1
#else
#define PORTABLE_UNALIGNED_ACCESS 0
#endif

#endif

/* `unaligned_read_16` might not be defined if endianness was not determined. */
#if !defined(unaligned_read_le16toh)

#include "pendian.h"

#ifndef UINT8_MAX
#include <stdint.h>
#endif

#if PORTABLE_UNALIGNED_ACCESS 

#define unaligned_read_16(p) (*(uint16_t*)(p))
#define unaligned_read_32(p) (*(uint32_t*)(p))
#define unaligned_read_64(p) (*(uint64_t*)(p))

#define unaligned_read_le16toh(p) le16toh(*(uint16_t*)(p))
#define unaligned_read_le32toh(p) le32toh(*(uint32_t*)(p))
#define unaligned_read_le64toh(p) le64toh(*(uint64_t*)(p))

#define unaligned_read_be16toh(p) be16toh(*(uint16_t*)(p))
#define unaligned_read_be32toh(p) be32toh(*(uint32_t*)(p))
#define unaligned_read_be64toh(p) be64toh(*(uint64_t*)(p))

#else

#define unaligned_read_le16toh(p)  (                                        \
        (((uint16_t)(((uint8_t *)(p))[0])) <<  0) |                         \
        (((uint16_t)(((uint8_t *)(p))[1])) <<  8))

#define unaligned_read_le32toh(p)  (                                        \
        (((uint32_t)(((uint8_t *)(p))[0])) <<  0) |                         \
        (((uint32_t)(((uint8_t *)(p))[1])) <<  8) |                         \
        (((uint32_t)(((uint8_t *)(p))[2])) << 16) |                         \
        (((uint32_t)(((uint8_t *)(p))[3])) << 24))

#define unaligned_read_le64toh(p)  (                                        \
        (((uint64_t)(((uint8_t *)(p))[0])) <<  0) |                         \
        (((uint64_t)(((uint8_t *)(p))[1])) <<  8) |                         \
        (((uint64_t)(((uint8_t *)(p))[2])) << 16) |                         \
        (((uint64_t)(((uint8_t *)(p))[3])) << 24) |                         \
        (((uint64_t)(((uint8_t *)(p))[4])) << 32) |                         \
        (((uint64_t)(((uint8_t *)(p))[5])) << 40) |                         \
        (((uint64_t)(((uint8_t *)(p))[6])) << 48) |                         \
        (((uint64_t)(((uint8_t *)(p))[7])) << 56))

#define unaligned_read_be16toh(p)  (                                        \
        (((uint16_t)(((uint8_t *)(p))[0])) <<  8) |                         \
        (((uint16_t)(((uint8_t *)(p))[1])) <<  0))

#define unaligned_read_be32toh(p)  (                                        \
        (((uint32_t)(((uint8_t *)(p))[0])) << 24) |                         \
        (((uint32_t)(((uint8_t *)(p))[1])) << 16) |                         \
        (((uint32_t)(((uint8_t *)(p))[2])) <<  8) |                         \
        (((uint32_t)(((uint8_t *)(p))[3])) <<  0))

#define unaligned_read_be64toh(p)  (                                        \
        (((uint64_t)(((uint8_t *)(p))[0])) << 56) |                         \
        (((uint64_t)(((uint8_t *)(p))[1])) << 48) |                         \
        (((uint64_t)(((uint8_t *)(p))[2])) << 40) |                         \
        (((uint64_t)(((uint8_t *)(p))[3])) << 32) |                         \
        (((uint64_t)(((uint8_t *)(p))[4])) << 24) |                         \
        (((uint64_t)(((uint8_t *)(p))[5])) << 16) |                         \
        (((uint64_t)(((uint8_t *)(p))[6])) <<  8) |                         \
        (((uint64_t)(((uint8_t *)(p))[7])) <<  0))

#if __LITTLE_ENDIAN__
#define unaligned_read_16(p) unaligned_read_le16toh(p)
#define unaligned_read_32(p) unaligned_read_le32toh(p)
#define unaligned_read_64(p) unaligned_read_le64toh(p)
#endif

#if __BIG_ENDIAN__
#define unaligned_read_16(p) unaligned_read_be16toh(p)
#define unaligned_read_32(p) unaligned_read_be32toh(p)
#define unaligned_read_64(p) unaligned_read_be64toh(p)
#endif

#endif

#endif

#endif /* PUNALIGNED_H */
