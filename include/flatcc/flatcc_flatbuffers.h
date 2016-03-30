#ifndef FLATCC_FLATBUFFERS_H
#define FLATCC_FLATBUFFERS_H

/*
 * Replace this file to redefine the default flatbuffer
 * types and endian detection, or use as is.
 */

#ifndef flatcc_flatbuffers_defined
#define flatcc_flatbuffers_defined

#ifdef FLATCC_PORTABLE
#include "flatcc/flatcc_portable.h"
#endif

#define __FLATBUFFERS_PASTE2(a, b) a ## b
#define __FLATBUFFERS_PASTE3(a, b, c) a ## b ## c
#define __FLATBUFFERS_CONCAT(a, b) __FLATBUFFERS_PASTE2(a, b)

/*
 * "flatcc_endian.h" requires the preceeding include files,
 * or compatible definitions.
 */
#include "flatcc/portable/pendian.h"
#include "flatcc/flatcc_types.h"
#include "flatcc/flatcc_endian.h"

/*
 * `static_assert` is used by generated structs to ensure that they have
 * the expected size in case some compiler disagrees on how to pack
 * structs.
 *
 * The portable wrapper is required by non-C11 compilers, and some C11
 * compilers that use a clib that is not fully compliant (e.g. Clang
 * OS-X). If all compilation is done with a compliant compiler, the
 * following may be removed as <assert.h> will include the definition
 * where relevant.
 */
#include "flatcc/portable/pstatic_assert.h"

#ifndef FLATBUFFERS_WRAP_NAMESPACE
#define FLATBUFFERS_WRAP_NAMESPACE(ns, x) ns ## _ ## x
#endif

#endif /* flatcc_flatbuffers_defined */

#endif /* FLATCC_FLATBUFFERS_H */
