#ifndef PSTDALIGN_H
#define PSTDALIGN_H

#ifndef __alignas_is_defined

#ifndef __cplusplus

#if (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) 
/* C11 or newer */
#include <stdalign.h>
#else

#if defined(__GNUC__) || defined (__IBMC__) || defined(__clang__)
#define _Alignas(t) __attribute__((__aligned__(t)))
#define _Alignof(t) __alignof__(t)
#elif defined(_MSC_VER)
#define _Alignas(t) __declspec (align(t))
#define _Alignof(t) __alignof(t)
#else
#error please update pstdalign.h with support for current compiler
#endif

#define alignas _Alignas
#define alignof _Alignof

#define __alignas_is_defined 1
#define __alignof_is_defined 1

#endif /* __STDC_VERSION__ */

#endif /* __cplusplus */
#endif /* __alignas__is_defined */
#endif /* PSTDALIGN_H */
