/*
 * Copyright (c) 2024 Mikkel Fahnøe Jørgensen, dvide.com
 *
 * (MIT License)
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

/* Provide strict aliasing safe portable access to reading and writing integer and
   floating point values from memory buffers of size 1, 2, 4, and 8 bytes, and also
   for casting between integers and floats at the binary level, e.g.
   mem_read_float32(&myuint32), which can be necessary for endian conversions.

   It is often suggested to use memcpy for this purpose, that is not an ideal
   solution. See comments below.

   While this is intended to be aligned access, the strict C aliasing rules forces
   this to be the same as unaligned access. On x86/64 we can be more relaxed both
   with aliasing and alignment, but if at some point a compiler starts to
   modify this behaviour, the header can be updated or PORTABLE_MEM_PTR_ACCESS
   can be defined to 0 in the build configuration, or this file can be updated. */


/* NOTE: for best performance, `__builtin_memcpy` should be detectable, but
   the current detection logic is not ideal for older compilers. See below. */


#ifndef PMEMACCESS_H
#define PMEMACCESS_H


#ifdef __cplusplus
extern "C" {
#endif


/* Set to 1 to see which implementation is chosen on current platform. */
#ifndef PORTABLE_MEM_ACCESS_DEBUG
#define PORTABLE_MEM_ACCESS_DEBUG 0
#endif

/* MEM_PTR_ACCESS (aka pointer casts) (*(T *)p) is not formally valid for strict aliasing.
   It works in most cases, but not always. It may be the best option for older
   compilers that do not optimize well and which don't care about strict aliazing.
   x86/64 platforms appears to work well with this, while it only sometimes work
   on other platforms.

   NOTE: this might change as compiler updates their optimization strategies. */

#ifndef PORTABLE_MEM_PTR_ACCESS

#if (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
#  define PORTABLE_MEM_PTR_ACCESS 1
#else
#  define PORTABLE_MEM_PTR_ACCESS 0
#endif

#endif /* PORTABLE_MEM_PTR_ACCESS */


#ifndef UINT8_MAX
#  include <stdint.h>
#endif

/* `mem_copy_word` implements optimized non-overlapping memory copies of 1, 2, 4, or 8.
   16 byte words are not guaranteed to be supported, but might be in the future.

   Ideally call `mem_copy_word` with known constant lengths for best optimization.

   The objective is both to support type punning where a binary representation is
   reinterpreted, and to read and write integers and floats safely from memory without
   risking undefined behaviour from strict aliasing rules.

   `memcpy` is supposed to handle this efficiently given small constant powers of 2, but
   this evidently fails on some platforms since even small constants lengths with -O2
   level optimization can issue a function call. On such platforms, `__builtin_memcpy`
   tends to work better, if it can be detected.

   A pointer cast can and will, albeit uncommon, lead to undefined behaviour
   such as reading from uninitialized stack memory when strict aliasing is the default
   optimization strategy.

   Note: __has_builtin is not necessarily defined on platforms with __builtin_memcpy support,
   so detection can be improved. Feel free to contribute. */

#ifndef mem_copy_word
#  if defined(__has_builtin)
#    if __has_builtin(__builtin_memcpy)
#      define mem_copy_word(d, s, n) __builtin_memcpy((d), (s), (n))
#      if PORTABLE_MEM_ACCESS_DEBUG
#        error mem_copy_word using: __builtin_memcpy
#      endif
#    endif
#  endif
#endif

#ifndef mem_copy_word
    #include "pinline.h"
    #include "prestrict.h"
    /* Sometimes `memcpy` is a call even when optimized and given small constant length arguments,
       so this is more likely to be optimized. `int len` is used to avoid <stddef.h> dependency.
       Also, by not using `memcpy`, we avoid depending on <stdlib.h>.

      As an alternative consider PORTABLE_MEM_PTR_ACCESS=1 with `mem_read/write_nn` for some older
      platforms that might not care about strict aliasing, and which also might not optimize well. */
    static inline void mem_copy_word(void * restrict dest, const void * restrict src, int len)
    {
        char *d = (char *)dest;
        const char *s = (const char *)src;
        while(len--) *d++ = *s++;
    }
    #if PORTABLE_MEM_ACCESS_DEBUG
    #  error mem_copy_word using: inline memcpy
    #endif
#endif

        
#if PORTABLE_MEM_PTR_ACCESS 

#define mem_read_8(p)  (*(uint8_t*)(p))
#define mem_read_16(p) (*(uint16_t*)(p))
#define mem_read_32(p) (*(uint32_t*)(p))
#define mem_read_64(p) (*(uint64_t*)(p))

#define mem_write_8(p, v)  ((void)(*(uint8_t*)(p) = (uint8_t)(v)))
#define mem_write_16(p, v) ((void)(*(uint16_t*)(p) = (uint16_t)(v)))
#define mem_write_32(p, v) ((void)(*(uint32_t*)(p) = (uint32_t)(v)))
#define mem_write_64(p, v) ((void)(*(uint64_t*)(p) = (uint64_t)(v)))

#define mem_read_float_32(p) (*(float*)(p))
#define mem_read_float_64(p) (*(double*)(p))

#define mem_write_float_32(p, v) ((void)(*(float*)(p) = (float)(v)))
#define mem_write_float_64(p, v) ((void)(*(double*)(p) = (double)(v)))

#if PORTABLE_MEM_ACCESS_DEBUG
#  error mem_read/write_nn using: pointer cast
#endif

#else /* PORTABLE_MEM_PTR_ACCESS */

#define mem_read_8(p)  (*(uint8_t*)(p))
#define mem_write_8(p, v)  ((void)(*(uint8_t*)(p) = (uint8_t)(v)))

static inline uint16_t mem_read_16(const void *p) { uint16_t v; mem_copy_word(&v, p, 2); return v; }
static inline uint32_t mem_read_32(const void *p) { uint32_t v; mem_copy_word(&v, p, 4); return v; }
static inline uint64_t mem_read_64(const void *p) { uint64_t v; mem_copy_word(&v, p, 8); return v; }

#define mem_write_16(p, v) do { const uint16_t x = (uint16_t)(v); mem_copy_word((p), &x, 2); } while(0)
#define mem_write_32(p, v) do { const uint32_t x = (uint32_t)(v); mem_copy_word((p), &x, 4); } while(0)
#define mem_write_64(p, v) do { const uint64_t x = (uint64_t)(v); mem_copy_word((p), &x, 8); } while(0)

static inline float mem_read_float_32(const void *p) { float v; mem_copy_word(&v, p, 4); return v; }
static inline double mem_read_float_64(const void *p) { double v; mem_copy_word(&v, p, 8); return v; }

#define mem_write_float_32(p, v) do { const float x = (float)(v); mem_copy_word((p), &x, 4); } while(0)
#define mem_write_float_64(p, v) do { const double x = (double)(v); mem_copy_word((p), &x, 8); } while(0)

#if PORTABLE_MEM_ACCESS_DEBUG
#  error mem_read/write_nn using: mem_copy_word
#endif

#endif /* PORTABLE_MEM_PTR_ACCESS */


#ifdef __cplusplus
}
#endif

#endif /* PMEMACCESS_H */
