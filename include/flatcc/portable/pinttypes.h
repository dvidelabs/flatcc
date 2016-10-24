#ifndef PINTTYPES_H
#define PINTTYPES_H

#if (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
#include <inttypes.h>
#else

/*
 * This is not a complete implementation of <inttypes.h>, just the most
 * useful printf modifiers.
 */

#include "pstdint.h"

#ifndef PRINTF_INT64_MODIFIER
#error "please define PRINTF_INT64_MODIFIER"
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

# endif /* __STDC__ */

#ifndef PRIszu
#ifndef PORTABLE_USE_DEPRECATED_INTTYPES
#error PRIsz? modifiers disabled. Requires mucn more work to be portable.
#endif
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


#endif /* PINTTYPES */
