#ifndef FLATCC_UNLIGNED_H
#define FLATCC_UNLIGNED_H

/*
 * This header provides a minimal safe detection for unligned access.
 * The portable/punaligned.h provides support for endian neutral
 * unligned reads and more, but sometimes we just need to know if it is
 * safe to grab a chunk of bytes from a pointer.
 */

#ifndef FLATCC_ALLOW_UNALIGNED_ACCESS

/*
 * If the portable layer is included, we might have more accurate
 * information.
 */
#ifdef PORTABLE_UNALIGNED_ACCESS

#define FLATCC_ALLOW_UNALIGNED_ACCESS PORTABLE_UNALIGNED_ACCESS

#else

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#define FLATCC_ALLOW_UNALIGNED_ACCESS 1
#else
#define FLATCC_ALLOW_UNALIGNED_ACCESS 0
#endif

#endif
#endif

#endif /* FLATCC_UNLIGNED_H */
