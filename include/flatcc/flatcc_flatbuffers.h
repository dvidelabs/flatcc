#ifndef FLATCC_FLATBUFFERS_H
#define FLATCC_FLATBUFFERS_H

#ifndef flatcc_flatbuffers_defined
#define flatcc_flatbuffers_defined

#ifdef FLATCC_PORTABLE
#include "flatcc/flatcc_portable.h"
#endif
#include "flatcc/portable/pwarnings.h"

/*
 * Even -std=c11 supporting compilers depend on clib support for
 * `static_assert` which isn't always present, so we deal with this here
 * for all compilers.
 */
#include "flatcc/portable/pstatic_assert.h"

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
#include "flatcc/flatcc_identifier.h"

#ifndef FLATBUFFERS_WRAP_NAMESPACE
#define FLATBUFFERS_WRAP_NAMESPACE(ns, x) ns ## _ ## x
#endif

#endif /* flatcc_flatbuffers_defined */

#endif /* FLATCC_FLATBUFFERS_H */
