#ifndef PDIAGNOSTIC_POP_H
#define PDIAGNOSTIC_POP_H

#if PDIAGNOSTIC_PUSHED_MSVC
#pragma warning( pop )
#undef PDIAGNOSTIC_PUSHED_MSVC
#elif PDIAGNOSTIC_PUSHED_CLANG
#pragma clang diagnostic pop
#undef PDIAGNOSTIC_PUSHED_CLANG
#elif PDIAGNOSTIC_PUSHED_GCC
#pragma GCC diagnostic pop
#undef PDIAGNOSTIC_PUSHED_GCC
#endif

#endif /* PDIAGNOSTIC_POP_H */
