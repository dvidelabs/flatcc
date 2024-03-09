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


/* Provide protable access to read and write memory buffers of 1, 2, 4, and 8 bytes.
   While this is intended to be aligned access, the strict C aliasing rules forces
   this to be the same as unaligned access. On x86/64 we be more relaxed both
   with aliasing and alignment, but if at some point a compiler starts to
   modify this behaviour, the header can be updated or PORTABLE_MEM_PTR_ACCESS
   can be defined to 0 in the build configuration. */


#ifndef PMEMACCESS_H
#define PMEMACCESS_H


#ifdef __cplusplus
extern "C" {
#endif


/* Set to 1 to see which implementation is chosen on current platform. */
#ifndef PORTABLE_MEM_ACCESS_DEBUG
#define PORTABLE_MEM_ACCESS_DEBUG 0
#endif

/* MEM_PTR_ACCESS (*(T *)p) is not strictly valid for strict aliasing, but is widely supported on x86/64
   but not always on ARM even if memory is aligned, so just opt out and hope for fast __builin_memcpy.
   This also sidesteps the requirement for aligned memory. */
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

/* `mem_copy_word` is intended to support non-overlapping memory copies of 0 to 8 bytes optimized.
   It is not necessarily defined above length 8, but it must adhere to strict aliasing rules.
   If a platform has a better option, it can be defined prior to inclusion of this header,
   and if so, the strict requirement may be relaxed based on platform knowledge and build flags.
   We do not require alignment because it is not possible with strict C aliasing rules.
   Everything must be funneled through a char or "similar narrow" type when formally strict. */
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
    /* Sometimes memcpy is a call, unnecessesarily, so this is more likely to be optimized.
       Use int for size to avoid stddef.h dependency. */
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

#if PORTABLE_MEM_ACCESS_DEBUG
#  error mem_read/write_nn using: mem_copy_word
#endif

#endif /* PORTABLE_MEM_PTR_ACCESS */


#ifdef __cplusplus
}
#endif

#endif /* PMEMACCESS_H */
