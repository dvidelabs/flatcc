#ifndef PSTDALIGN_H
#define PSTDALIGN_H

#ifndef __alignas_is_defined
#ifndef __cplusplus

#if ((__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 7)) || defined(__IBMC__)
#undef PORTABLE_C11_STDALIGN_MISSING
#define PORTABLE_C11_STDALIGN_MISSING
#endif

#if (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) && \
    !defined(PORTABLE_C11_STDALIGN_MISSING)
/* C11 or newer */
#include <stdalign.h>

#else

#if defined(__GNUC__) || defined (__IBMC__) || defined(__clang__)
#define _Alignas(t) __attribute__((__aligned__(t)))
#define _Alignof(t) __alignof__(t)
#elif defined(_MSC_VER)
#define _Alignas(t) __declspec (align(t))
#define _Alignof(t) __alignof(t)
#define alignas _Alignas
#define alignof _Alignof

#define __alignas_is_defined 1
#define __alignof_is_defined 1

#else
#error please update pstdalign.h with support for current compiler
#endif

#endif /* __STDC__ */

#endif /* __cplusplus */
#endif /* __alignas__is_defined */
#endif /* PSTDALIGN_H */
