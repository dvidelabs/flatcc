#ifndef PORTABLE_H
#define PORTABLE_H
#include <assert.h>
#if (__STDC_VERSION__ < 201112L) && !defined (PORTABLE_STANDALONE)
#include "pstdint.h"
#include "pstdalign.h"
#include "pinline.h"
#include "static_assert.h"
#include "pendian.h"
#else
#include <stdint.h>
#include <stdalign.h>
#ifndef static_assert
#define static_assert(pred, msg) _Static_assert(pred, msg)
#endif
#endif
#endif /* PORTABLE_H */
