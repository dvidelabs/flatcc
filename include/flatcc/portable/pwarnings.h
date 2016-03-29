#ifndef PWARNINGS_H
#define PWARNINGS_H

#if defined(_MSC_VER)
/* Needed when flagging code in or out and more. */
#pragma warning(disable: 4127) /* conditional expression is constant */
/* happens also in MS's own headers. */
#pragma warning(disable: 4668) /* preprocessor name not defined */
/* MSVC does not respect double parenthesis for intent */
#pragma warning(disable: 4706) /* assignment within conditional expression */
/* `inline` only advisory anyway. */
#pragma warning(disable: 4710) /* function not inlined */
/* Well, we don't intend to add the padding manually. */
#pragma warning(disable: 4820) /* x bytes padding added in struct */

/* Define this in the build as `-D_CRT_SECURE_NO_WARNINGS`, it has no effect here. */
/* #define _CRT_SECURE_NO_WARNINGS don't warn that fopen etc. are unsafe */

/*
 * Anonymous union in struct is valid in C11 and has been supported in
 * GCC and Clang for a while, but it is not C99. MSVC also handles it,
 * but warns. Truly portable code should perhaps not use this feature,
 * but this is not the place to complain about it.
 */
#pragma warning(disable: 4201) /* nonstandard extension used: nameless struct/union */
#endif

#endif PWARNINGS_H
