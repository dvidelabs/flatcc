#ifndef PORTABLE_H
#define PORTABLE_H
#include <assert.h>
#if (__STDC_VERSION__ < 201112L) && !defined (PORTABLE_STANDALONE)
#include "pinttypes.h"
#include "pstdalign.h"
#include "pinline.h"
#include "static_assert.h"
#else
#include <inttypes.h>
#include <stdalign.h>
#ifndef static_assert
#define static_assert(pred, msg) _Static_assert(pred, msg)
#endif
#endif

#include "pendian.h"

#endif /* PORTABLE_H */
