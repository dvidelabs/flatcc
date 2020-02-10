#ifndef FLATCC_ASSERT_H
#define FLATCC_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*
* These assert abstraction is __only__ for runtime libraries.
*
* The flatcc compiler uses Posix assert routines regardless
* of how this file is configured.
*
* This header makes it possible to use systems where assert is not
* valid to use.
*
* `FLATCC_ASSERT` is designed to handle errors which cannot be ignored
* and could lead to crash.
*
* `FLATCC_ASSERT` can be overrided by preprocessor definition
*
* If `FLATCC_ASSERT` definition is not provided, posix assert is being used
*/

#ifdef FLATCC_NO_ASSERT 
/* NOTE: This will not affect inclusion of <assert.h> for static assertions. */
#undef FLATCC_ASSERT
#define FLATCC_ASSERT(x) ((void)0)
/* Grisu3 is used for floating point conversion in JSON processing. */
#define GRISU3_NO_ASSERT
#endif

#ifndef FLATCC_ASSERT
#include <assert.h>
#define FLATCC_ASSERT assert
#endif

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_ASSERT_H */
