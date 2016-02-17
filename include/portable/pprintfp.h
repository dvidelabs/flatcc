#ifndef PPRINTFP_H
#define PPRINTFP_H

/* 
 * Grisu3 is not part of the portable lib per se, it must be in the
 * include path. Grisu3 provides much faster printing and parsing in the
 * typical case with fallback to sprintf for printing and strod for
 * parsing.
 *
 * Either define PORTABLE_USE_GRISU3, or include the grisu3 header first.
 *
 * Without grisu3, this file still ensures that a float or a double is
 * printed without loss of precision.
 */
#if PORTABLE_USE_GRISU3 
#include "grisu3/grisu3_print.h"
#endif

#ifdef grisu3_print_double_is_defined
/* Currently there is not special support for floats. */
#define print_float(n, p) grisu3_print_double((float)(n), (p))
#define print_double(n, p) grisu3_print_double((double)(n), (p))
#else
#include <stdio.h>
#define print_float(n, p) sprintf(p, "%.9g", (float)(n))
#define print_double(n, p) sprintf(p, "%.17g", (double)(n))
#endif

#define print_hex_float(n, p) sprintf(p, "%a", (float)(n))
#define print_hex_double(n, p) sprintf(p, "%a", (double)(n))

#endif /* PPRINTFP_H */
