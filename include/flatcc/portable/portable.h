#ifndef PORTABLE_H
#define PORTABLE_H

#include "pversion.h"
#include "pwarnings.h"
#include <assert.h>

#if !defined(PORTABLE_STANDALONE) && \
    (defined(__STDC__) && __STDC__ && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) 
/* C11 or newer */
#include <inttypes.h>
#include <stdalign.h>
#ifndef static_assert
#define static_assert(pred, msg) _Static_assert(pred, msg)
#else
#include "pinttypes.h"
#include "pstdalign.h"
#include "pinline.h"
#include "pstatic_assert.h"
#endif
#endif

#include "pendian.h"
#include "punaligned.h"

#endif /* PORTABLE_H */
