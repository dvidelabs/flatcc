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

#define __PSTATIC_ASSERT_CONCAT_(a, b) static_assert_scope_##a##_line_##b
#define __PSTATIC_ASSERT_CONCAT(a, b) __PSTATIC_ASSERT_CONCAT_(a, b)
#ifdef __COUNTER__
#define static_assert(e, msg) enum { __PSTATIC_ASSERT_CONCAT(__COUNTER__, __LINE__) = 1/(!!(e)) }
#else
#include "pstatic_assert_scope.h"
#define static_assert(e, msg) enum { __PSTATIC_ASSERT_CONCAT(__PSTATIC_ASSERT_COUNTER, __LINE__) = 1/(!!(e)) }
#endif

#endif /* __has_feature */
#endif /* static_assert */

#endif /* PSTATIC_ASSERT_H */

/* Update scope counter outside of include guard. */
#ifdef __PSTATIC_ASSERT_COUNTER
#include "pstatic_assert_scope.h"
#endif
