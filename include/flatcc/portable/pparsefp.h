#ifndef PPARSEFP_H
#define PPARSEFP_H

/*
 * Parses a float or double number and returns the length parsed if
 * successful. The length argument is of limited value due to dependency
 * on `strtod` - buf[len] must be accessible and must not be part of
 * a valid number, including hex float numbers..
 *
 * Unlike strtod, whitespace is not parsed.
 *
 * May return:
 * - null on error,
 * - buffer start if first character does not start a number,
 * - or end of parse on success.
 *
 */

#define PDIAGNOSTIC_IGNORE_UNUSED_FUNCTION
#include "pdiagnostic_push.h"

/*
 * We don't generally use HAVE_ macros, but isinf requires linking with
 * libmath, so make it possible to is it instead of fallback if desired.
 */
#if defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER) || defined(HAVE_ISINF)
#include <math.h>
#if defined(_MSC_VER) && !defined(isinf)
#include <float.h>
#define isnan _isnan
#define isinf(x) (!_finite(x))
#endif
#define parse_double_isinf isinf
#define parse_float_isinf isinf
#else

#ifndef UINT8_MAX
#include <stdint.h>
#endif

/* Avoid linking with libmath but depends on float/double being IEEE754 */
static inline int parse_double_isinf(double x)
{
    union { uint64_t u64; double f64; } v;
    v.f64 = x;
    return (v.u64 & 0x7fffffff00000000ULL) == 0x7ff0000000000000ULL;
}

static inline int parse_float_isinf(float x)
{
    union { uint32_t u32; float f32; } v;
    v.f32 = x;
    return (v.u32 & 0x7fffffff) == 0x7f800000;
}
#endif

/* Positive isinf is overflow, negative isinf is underflow. */
static inline int parse_double_is_range_error(double x)
{
    return parse_double_isinf(x);
}

static inline int parse_float_is_range_error(float x)
{
    return parse_float_isinf(x);
}

#ifndef PORTABLE_USE_GRISU3
#define PORTABLE_USE_GRISU3 1
#endif

#if PORTABLE_USE_GRISU3
#include "grisu3_parse.h"
#endif

#ifdef grisu3_parse_double_is_defined
static inline const char *parse_double(const char *buf, int len, double *result)
{
    return grisu3_parse_double(buf, len, result);
}
#else
#include <stdio.h>
static inline const char *parse_double(const char *buf, int len, double *result)
{
    char *end;

    (void)len;
    *result = strtod(buf, &end);
    return end;
}
#endif

static inline const char *parse_float(const char *buf, int len, float *result)
{
    const char *end;
    double v;

    end = parse_double(buf, len, &v);
    *result = (float)v;
    return end;
}

#include "pdiagnostic_pop.h"
#endif /* PPARSEFP_H */
