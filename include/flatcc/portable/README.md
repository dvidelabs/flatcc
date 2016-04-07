A small library for adding C11 compatibility to older C compilers, but
only a small highly useful subset such as static assertions, inline
functions and alignment.

Many compilers already have the required functionality but named
slightly different.

In addition, compatibility with the Linux `<endian.h>` system file is
provided, and "punaligned.h" is provided for unaligned memory reads
which in part depends on endian support.

The library also provides fast integer printing and floating point
printing and parsing optionally using the grisu3 algorithm, but can fall
back to strtod and related. The `pgrisu3` folder is header only and
excludes test cases found in the main grisu3 project the files were
extracted from.

Files can be included indvidually, or portable.h may be included to get
all functionality. If the compiler is C11 compliant, portable.h will not
include anything, except: it will provide a patch for static assertions
which clang does not fully support in all versions even with C11 flagged.

The grisu3 header files are the runtime files for the Grisu3 floating
point conversion to/from text C port. Test coverage is provided separately.
This library can be used indirectly via pparsefp.h and pprintfp.h.

IMPORTANT NOTE: this library is not widely tested. It is intended to be
a starting point. Each use case should test on relevant target platforms
and if relevant send patches upstream.

