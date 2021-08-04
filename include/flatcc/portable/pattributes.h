
/*
 * There are no standard attributes defines as macros.
 *
 * C23 introduces an attributes `[[<attribute>]]`. Priot to that
 * some compiler versions supported the `__attribute__` syntax.
 *
 * See also:
 * https://en.cppreference.com/w/c/language/attributes
 *
 * There is no portable way to use C23 attributes in older C standards
 * so in order to use these portably, some macro name needs to be
 * defined for each attribute.
 *
 * The Linux kernel defines certain attributes as macros, such as
 * `fallthrough`. When adding attributes is seems reasonable to follow
 * the Linux conventions in lack of any official standard. However, it
 * is not the intention that this file should mirror the Linux
 * attributes 1 to 1.
 *
 * See also:
 * https://github.com/torvalds/linux/blob/master/include/linux/compiler_attributes.h
 *
 * There is a risk that attribute macros may lead to conflicts with
 * other symbols. It is suggested to undefine such symbols after
 * including this header if that should become a problem.
 */


#ifndef PATTRIBUTES_H
#define PATTRIBUTES_H

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __has_c_attribute
# define PORTABLE_HAS_C_ATTRIBUTE(x) __has_c_attribute(x)
#else
# define PORTABLE_HAS_C_ATTRIBUTE(x) 0
#endif

#ifdef __has_attribute
# define PORTABLE_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
# define PORTABLE_HAS_ATTRIBUTE(x) 0
#endif


/* https://en.cppreference.com/w/c/language/attributes/fallthrough */
#ifndef fallthrough
# if PORTABLE_HAS_C_ATTRIBUTE(__fallthrough__)
#  define fallthrough [[__fallthrough__]]
# elif PORTABLE_HAS_ATTRIBUTE(__fallthrough__)
#  define fallthrough __attribute__((__fallthrough__))
# else
#  define fallthrough ((void)0)
# endif
#endif


#ifdef __cplusplus
}
#endif

#endif /* PATTRIBUTES_H */
