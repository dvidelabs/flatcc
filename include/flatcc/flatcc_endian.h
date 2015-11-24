#ifndef FLATCC_ENDIAN_H
#define FLATCC_ENDIAN_H

/*
 * This files does not cover all endian cases, but attempts to support
 * major platforms. It provides extra facilities not available to the
 * `common_....h` generated files by also attempting to support be32toh
 * and friends without portability layer which requries OS platform
 * detection and support. This means the runtime library (for which this
 * is intended) can support more features in this respect than
 * standalone generated code.
 */

#include "flatcc_types.h"

/*
 * For undetected platforms, provide a custom <endian.h> file in the include path,
 * or include equivalent functionality (the 6 lexxtoh, htolexx functions) before this file.
 */
#if !defined(FLATBUFFERS_LITTLEENDIAN)
#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) ||\
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||\
      (defined(_BYTE_ORDER) && _BYTE_ORDER == _LITTLE_ENDIAN) ||\
    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) ||\
    /* for MSVC */ defined(_M_X64) || defined(_M_IX86) || defined(_M_I86) ||\
    defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define FLATBUFFERS_LITTLEENDIAN 1
#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) ||\
      (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) ||\
      (defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN) ||\
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) ||\
    defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#define FLATBUFFERS_LITTLEENDIAN 0
#endif
#endif
#if defined(flatbuffers_is_native_pe)
/* NOP */
#else

#if (defined(__APPLE__) && defined(__MACH__))

#include <sys/types.h>
#include <libkern/OSByteOrder.h>

#if !defined(le16toh)
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define htole64(x) OSSwapHostToLittleInt64(x)

#define be16toh(x) OSSwapBigToHostInt16(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#endif

#elif defined(OS_FREEBSD) || defined(OS_OPENBSD) || defined(OS_NETBSD) || defined(OS_DRAGONFLYBSD)
#include <sys/types.h>
#include <sys/endian.h>
#elif defined(__linux__)
#include <endian.h>
#elif !defined(le16toh) && FLATBUFFERS_LITTLEENDIAN
#define le16toh(n) ((uint16_t)(n))
#define le32toh(n) ((uint32_t)(n))
#define le64toh(n) ((uint64_t)(n))
#define htole16(n) ((uint16_t)(n))
#define htole32(n) ((uint32_t)(n))
#define htole64(n) ((uint64_t)(n))

/* This is little endian. Use portability layer for big endian. */
#if defined(__WINDOWS__)
#include <stdlib.h>
#define be16toh(n) _byteswap_ushort(n)
#define be32toh(n) _byteswap_ulong(n)
#define be64toh(n) _byteswap_uint64(n)
#define htobe16(n) _byteswap_ushort(n)
#define htobe32(n) _byteswap_ulong(n)
#define htobe64(n) _byteswap_uint64(n)
#endif

#else

/*
 * Use portable layer if needed, it has more complete platform neutral
 * swapping and detection logic.
 */

/* Just a wild guess, and to allow user to provide a custom file. */
#include <endian.h>
#endif

#if defined(letoh16) && !defined(le16toh)
#define le16toh letoh16
#define le32toh letoh32
#define le64toh letoh64
#endif
#endif
#ifndef flatbuffers_is_native_pe
#define flatbuffers_is_native_pe() (le16toh(1) == 1)
#endif

#ifndef flatbuffers_is_native_le
#define flatbuffers_is_native_le() flatbuffers_is_native_pe()
#endif

#ifndef flatbuffers_is_native_be
#define flatbuffers_is_native_be() (!flatbuffers_is_native_pe())
#endif

#if defined(__LITTLE_ENDIAN__) || FLATBUFFERS_LITTLEENDIAN

/* pe indicates protocol endian, which for FlatBuffers is little endian. */
#undef flatbuffers_is_native_le
#define flatbuffers_is_native_le() 1

#undef flatbuffers_is_native_be
#define flatbuffers_is_native_be() 0

#undef flatbuffers_is_native_pe
#define flatbuffers_is_native_pe flatbuffers_is_le

#undef flatbuffers_store_uoffset
#undef flatbuffers_store_voffset
#undef flatbuffers_load_uoffset
#undef flatbuffers_load_voffset

#define flatbuffers_store_uoffset(n) (n)
#define flatbuffers_store_voffset(n) (n)
#define flatbuffers_load_uoffset(n) (n)
#define flatbuffers_load_voffset(n) (n)

#else

#undef flatbuffers_is_native_le
#define flatbuffers_is_native_le() 1

#undef flatbuffers_is_native_be
#define flatbuffers_is_native_be() 0

#undef flatbuffers_is_native_pe
#define flatbuffers_is_native_pe flatbuffers_is_le

#define __FLATCC_ENDIAN_PASTE_2(A, B) A ## B
#define __FLATCC_ENDIAN_PASTE_3(A, B, C) A ## B ## C
#define __FLATCC_ENDIAN_STORE(W, n) __FLATCC_ENDIAN_PASTE_2(htole, W)(n)
#define __FLATCC_ENDIAN_LOAD(W, n) __FLATCC_ENDIAN_PASTE_3(le, W, toh)(n)

#define flatbuffers_store_uoffset(n) (__FLATCC_ENDIAN_STORE(FLATBUFFERS_UOFFSET_WIDTH, n))
#define flatbuffers_store_voffset(n) (__FLATCC_ENDIAN_STORE(FLATBUFFERS_VOFFSET_WIDTH, n))
#define flatbuffers_load_uoffset(n) (__FLATCC_ENDIAN_LOAD(FLATBUFFERS_UOFFSET_WIDTH, n))
#define flatbuffers_load_voffset(n) (__FLATCC_ENDIAN_LOAD(FLATBUFFERS_VOFFSET_WIDTH, n))

#endif

#endif /* FLATCC_ENDIAN_H */
