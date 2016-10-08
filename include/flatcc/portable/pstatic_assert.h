#ifndef PSTATIC_ASSERT_H
#define PSTATIC_ASSERT_H

#include <assert.h>

#ifndef static_assert

#ifndef __has_feature         
  #define __has_feature(x) 0
#endif

#if __has_feature(c_static_assert)

#define static_assert(pred, msg) _Static_assert(pred, msg)

#else

#define __PSTATIC_ASSERT_CONCAT_(a, b) a##b
#define __PSTATIC_ASSERT_CONCAT(a, b) __PSTATIC_ASSERT_CONCAT_(a, b)
#ifdef __COUNTER__
#define static_assert(e, msg) enum { __PSTATIC_ASSERT_CONCAT(static_assertion_, __COUNTER__) = 1/(!!(e)) }
#else
#define static_assert(e, msg) enum { __PSTATIC_ASSERT_CONCAT(static_assert_line_, __LINE__) = 1/(!!(e)) }
#endif

#endif /* __has_feature */
#endif /* static_assert */

#endif /* PSTATIC_ASSERT_H */
