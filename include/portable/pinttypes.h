#ifndef PINTTYPES_H
#define PINTTYPES_H

#if 0 || (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#include <inttypes.h>
#else

/*
 * This is not a complete implementation of <inttypes.h>, just the most
 * useful printf modifiers.
 */

#include "pstdint.h"

/*
 * Work around pstdint.h issue on x64 OS-X 10.11 clang that defines the
 * modifier to "l" which generates warnings
 *
 * Note: with the __STD__ test above inttypes.h would normally be used
 * instead.
 */
#if __clang__
#undef PRINTF_INT64_MODIFIER
#endif

#ifndef PRINTF_INT64_MODIFIER
#ifdef _MSC_VER
#define PRINTF_INT64_MODIFIER "I64"
#else
#define PRINTF_INT64_MODIFIER "ll"
#endif
#endif

#ifndef PRId64
#define PRId64 PRINTF_INT64_MODIFIER "d"
#define PRIu64 PRINTF_INT64_MODIFIER "u"
#define PRIx64 PRINTF_INT64_MODIFIER "x"
#endif

#ifndef PRINTF_INT32_MODIFIER
#define PRINTF_INT32_MODIFIER "l"
#endif

#ifndef PRId32
#define PRId32 PRINTF_INT32_MODIFIER "d"
#define PRIu32 PRINTF_INT32_MODIFIER "u"
#define PRIx32 PRINTF_INT32_MODIFIER "x"
#endif

#ifndef PRINTF_INT16_MODIFIER
#define PRINTF_INT16_MODIFIER "h"
#endif

#ifndef PRId16
#define PRId16 PRINTF_INT16_MODIFIER "d"
#define PRIu16 PRINTF_INT16_MODIFIER "u"
#define PRIx16 PRINTF_INT16_MODIFIER "x"
#endif

#ifndef PRIszu
#ifdef _MSC_VER
  #define PRIszd "Id"
  #define PRIszu "Iu"
  #define PRIszx "Ix"
  #define PRIpdu "Iu"
  #define PRIpdd "Id"
  #define PRIpdx "Ix"
#else
  #define PRIszd "zd"
  #define PRIszu "zu"
  #define PRIszx "zx"
  #define PRIpdd "td"
  #define PRIpdu "tu"
  #define PRIpdx "tx"
#endif
#endif

# endif /* __STDC__ */

#endif /* PINTTYPES */
