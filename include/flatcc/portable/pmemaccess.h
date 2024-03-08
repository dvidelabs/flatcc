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


/* MEM_PTR_ACCESS (*(T *)p) is not strictly valid for strict aliasing, but is widely supported on x86/64
   but not always on ARM even if memory is aligned, so just opt out and hope for fast __builin_memcpy.
   This also sidesteps the requirement for aligned memory. */
#ifndef PORTABLE_MEM_PTR_ACCESS

#if (defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64))
#define PORTABLE_MEM_PTR_ACCESS 1
#else
#define PORTABLE_MEM_PTR_ACCESS 0
#endif

#endif

#ifndef UINT8_MAX
#include <stdint.h>
#endif

/* mem_copy_word is intended to support memory copies of 0 to 8 bytes optimized.
   it is not necessarily defined above length 8, but it must adhere to strict aliasing rules.
   if a platform has a better option, it can be defined prior to inclusion of this header.
   We do not require alignment because it isn't possible with strict C aliasing rules.
   Everything must be funneled through a char or "similar narrow" type. */
#ifndef mem_copy_word
#if defined(__has_builtin) && __has_builtin(__builtin_memcpy)
    #define mem_copy_word(d, s, n) __builtin_memcpy((d), (s), (n))
#endif
#endif

#ifndef mem_copy_word
    #include "pinline.h"
    #include "prestrict.h"
    /* Sometimes memcpy is a call, unnecessarily, so this is more likely to be optimized. */
    static inline void mem_copy_word(void * restrict dest, const void * restrict src, size_t len)
    {
        const char *d = dest, *s = src;
        while(len--) *d++ = *s++;
    }
#endif

        
#if PORTABLE_MEM_PTR_ACCESS 

#define mem_read_8(p)  (*(uint8_t*)(p))
#define mem_read_16(p) (*(uint16_t*)(p))
#define mem_read_32(p) (*(uint32_t*)(p))
#define mem_read_64(p) (*(uint64_t*)(p))

#define mem_write_8(p, v)  ((void)(*(uint8_t*)(p) = (v)))
#define mem_write_16(p, v) ((void)(*(uint16_t*)(p) = (v)))
#define mem_write_32(p, v) ((void)(*(uint32_t*)(p) = (v)))
#define mem_write_64(p, v) ((void)(*(uint64_t*)(p) = (v)))

#else

#define mem_read_8(p)  (*(uint8_t*)(p))
#define mem_read_16(p) static inline uint16_t(const void *p) { uint16_t v; mem_copy_word(&v, p, 2); return v; }
#define mem_read_32(p) static inline uint32_t(const void *p) { uint32_t v; mem_copy_word(&v, p, 4); return v; }
#define mem_read_64(p) static inline uint32_t(const void *p) { uint64_t v; mem_copy_word(&v, p, 8); return v; }

#define mem_write_8(p, v)  ((void)(*(uint8_t*)(p) = (v)))

#define mem_write_16(p, v) mem_copy_word((p), &(v), 2)
#define mem_write_32(p, v) mem_copy_word((p), &(v), 4)
#define mem_write_64(p, v) mem_copy_word((p), &(v), 8)

#endif

#ifdef __cplusplus
}
#endif

#endif /* PMEMACCESS_H */
