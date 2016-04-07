 /* There is intentionally no include guard in this file. */


/*
 * Usage: optionally disable any of these before including.
 *
 * #define PDIAGNOSTIC_IGNORE_UNUSED_FUNCTION
 * #define PDIAGNOSTIC_IGNORE_UNUSED_VARIABLE
 * #define PDIAGNOSTIC_IGNORE_UNUSED_PARAMETER
 * #define PDIAGNOSTIC_IGNORE_UNUSED // all of the above
 *
 * #include "pdiagnostic.h"
 *
 * Alternatively use #include "pdiagnostic_push/pop.h"
 */

#ifdef _MSC_VER
#pragma warning(disable: 4668) /* preprocessor name not defined */
#endif

#if defined(_MSC_VER) && !defined(PDIAGNOSTIC_AWARE_MSVC)
#define PDIAGNOSTIC_AWARE_MSVC 1
#elif defined(__clang__) && !defined(PDIAGNOSTIC_AWARE_CLANG)
#define PDIAGNOSTIC_AWARE_CLANG 1
/* Can disable some warnings even if push is not available (gcc-4.2 vs gcc-4.7) */
#elif ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)) && \
        !defined(PDIAGNOSTIC_AWARE_GCC)
#define PDIAGNOSTIC_AWARE_GCC 1
#endif

#if defined(PDIAGNOSTIC_IGNORE_UNUSED_FUNCTION) || defined(PDIAGNOSTIC_IGNORE_UNUSED)
#if PDIAGNOSTIC_AWARE_CLANG
#pragma clang diagnostic ignored "-Wunused-function"
#elif PDIAGNOSTIC_AWARE_GCC
#pragma GCC diagnostic ignored "-Wunused-function" */
#endif
#endif
#undef PDIAGNOSTIC_IGNORE_UNUSED_FUNCTION

#if defined(PDIAGNOSTIC_IGNORE_UNUSED_VARIABLE) || defined(PDIAGNOSTIC_IGNORE_UNUSED)
#if PDIAGNOSTIC_AWARE_MSVC
#pragma warning(disable: 4101) /* unused local variable */
#elif PDIAGNOSTIC_AWARE_CLANG
#pragma clang diagnostic ignored "-Wunused-variable"
#elif PDIAGNOSTIC_AWARE_GCC
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#endif
#undef PDIAGNOSTIC_IGNORE_UNUSED_VARIABLE

#if defined(PDIAGNOSTIC_IGNORE_UNUSED_PARAMETER) || defined(PDIAGNOSTIC_IGNORE_UNUSED)
#if PDIAGNOSTIC_AWARE_CLANG
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif PDIAGNOSTIC_AWARE_GCC
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif
#endif
#undef PDIAGNOSTIC_IGNORE_UNUSED_PARAMETER

#undef PDIAGNOSTIC_IGNORE_UNUSED

