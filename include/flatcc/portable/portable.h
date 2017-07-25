#ifndef FLATCC_PORTABLE_H
#define FLATCC_PORTABLE_H

#define  flatcc_alloc(x)      malloc(x)
#define  flatcc_realloc(x,y)  realloc(x,y)
#define  flatcc_free(x)       free(x)

#include "pversion.h"
#include "pwarnings.h"

/* Featutures that ought to be supported by C11, but some aren't. */
#include "pinttypes.h"
#include "pstdalign.h"
#include "pinline.h"
#include "pstatic_assert.h"

/* These are not supported by C11 and are general platform abstractions. */
#include "pendian.h"
#include "punaligned.h"

#endif /* FLATCC_PORTABLE_H */
