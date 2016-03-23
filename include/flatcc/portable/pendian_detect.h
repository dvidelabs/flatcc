/*
 * Uses various known flags to decide endianness and defines:
 *
 * __LITTLE_ENDIAN__ or __BIG_ENDIAN__ if not already defined
 *
 * and also defines
 *
 * __BYTE_ORDER__ to either __ORDER_LITTLE_ENDIAN__ or
 * __ORDER_BIG_ENDIAN if not already defined
 *
 * If none of these could be set, __UNKNOWN_ENDIAN__ is defined,
 * which is not a known flag. If __BYTE_ORDER__ is defined but
 * not big or little endian, __UNKNOWN_ENDIAN__ is also defined.
 */

#ifndef PENDIAN_DETECT
#define PENDIAN_DETECT

#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__ 1234
#endif

#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__ 4321
#endif

#ifdef __BYTE_ORDER__

#if defined(__LITTLE_ENDIAN__) && __BYTE_ORDER != __ORDER_BIG_ENDIAN
#error __LITTLE_ENDIAN__ inconsistent with __BYTE_ORDER__
#endif

#if defined(__BIG_ENDIAN__) && __BYTE_ORDER != __ORDER_BIG_ENDIAN
#error __BIG_ENDIAN__ inconsistent with __BYTE_ORDER__
#endif

#else /* __BYTE_ORDER__ */

#if                                                                         \
  defined (__LITTLE_ENDIAN__) ||                                            \
  defined(__ARMEL__) || defined(THUMBEL__) || defined (__AARCH64EL__) ||    \
  defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)

#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__

#endif

#if                                                                         \
  defined (__BIG_ENDIAN__) ||                                               \
  defined(__ARMEB__) || defined(THUMBEB__) || defined (__AARCH64EB__) ||    \
  defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)

#define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__

#endif

#endif /* __BYTE_ORDER__ */

#ifdef __BYTE_ORDER__

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__ 1
#endif

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__ 1
#endif

#else

/*
 * Custom extension - we only define __BYTE_ORDER if known big or little.
 * User code that understands __BYTE_ORDER__ may also assume unkown if
 * it is not defined by know - this will allow other endian formats than
 * big or little when supported by compiler.
 */
#ifndef __UNKNOWN_ENDIAN__
#define __UNKNOWN_ENDIAN__ 1
#endif

#endif
#endif /* __BYTE_ORDER__ */

#if defined(__LITTLE_ENDIAN__) && defined(__BIG_ENDIAN__)
#error conflicting definitions of __LITTLE_ENDIAN__ and __BIG_ENDIAN__
#endif

#endif /* PENDIAN_DETECT */
