#ifndef STATIC_ASSERT_H
#define STATIC_ASSERT_H

#ifdef __GNUC__
#define __PSTATIC_ASSERT_UNUSED __attribute__((unused))
#else
#define __PSTATIC_ASSERT_UNUSED 
#endif

/*
 * NOTE: Some versions of clang support _Static_assert while
 * <assert.h> fails to define "static_assert"
 * In that case it may be better to compile with
 *
 * -Dstatic_assert=_Static_assert
 *
 * rather than using this file to remain within the standards.
 */


#include <assert.h>
#ifndef static_assert

#ifdef __has_feature
#if __has_feature(c_static_assert)
#define static_assert(pred, msg) _Static_assert(pred, msg)
#endif
#else
/*
 * __COUNTER__ is better than __FILE__ as it won't conflict,
 * but isn't always supported.
 */
#define __PSTATIC_ASSERT1(x, y) x ## y
#define __PSTATIC_ASSERT2(x, y) __PSTATIC_ASSERT1(x, y)
/* Switch condition conflicts compile time of predicate fails. */
#ifdef __COUNTER__
#define static_assert(pred, msg)                                            \
  __PSTATIC_ASSERT_UNUSED static void                                       \
__PSTATIC_ASSERT2(static_assert_, __LINE__)(void)                           \
  {switch(0){case 0:case pred:;}} /* static assertion failed */
#else
#define static_assert(pred, msg)                                            \
  __PSTATIC_ASSERT_UNUSED static void                                       \
  __PSTATIC_ASSERT2(static_assert_, __COUNTER__)(void)                      \
  {switch(0){case 0:case pred:;}} /* static assertion failed */
#endif

#endif /* __has_feature */
#endif /* static_assert */

#endif /* STATIC_ASSERT_H */
