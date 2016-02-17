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
 * - buffer start if first character does start a number,
 * - or end of parse on success.
 *
 * Grisu3 is not part of the portable lib per se, it must be in the
 * include path. Grisu3 provides much faster printing and parsing in the
 * typical case with fallback to sprintf for printing and strod for
 * parsing.
 *
 * Either define PORTABLE_USE_GRISU3, or include the grisu3 header first.
 */
#include <math.h> /* for HUGE_VAL */

#if PORTABLE_USE_GRISU3
#include "grisu3/grisu3_parse.h"
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

#endif /* PPARSEFP_H */
