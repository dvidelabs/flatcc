#ifndef FLATCC_ACCESSORS
#define FLATCC_ACCESSORS

#ifdef __cplusplus
extern "C" {
#endif

#ifndef UINT8_MAX
#include <stdint.h>
#endif
#include <string.h>

#define __flatcc_basic_scalar_accessors_impl(N, T, W, E)                    \
static inline size_t N ## __size(void)                                      \
{ return sizeof(T); }                                                       \
static inline T *N ## __ptr_add(T *p, size_t i)                             \
{ return p + i; }                                                           \
static inline const T *N ## __const_ptr_add(const T *p, size_t i)           \
{ return p + i; }                                                           \
static inline T N ## _read(const void *p)                                   \
{ return *(T *)p; }                                                         \
static inline void N ## _write(void *p, T v)                                \
{ *(T *)p = v; }

#define __flatcc_define_integer_accessors_impl(N, T, W, E)                  \
static inline T N ## _read_from_pe(const void *p)                           \
{ uint ## W ## _t u = *(uint ## W ## _t *)p;                                \
  u = E ## W ## toh(u); return (T)u; }                                      \
static inline void N ## _write_to_pe(void *p, T v)                          \
{ uint ## W ## _t u = (uint ## W ## _t)v;                                   \
  *(uint ## W ## _t *)p = hto ## E ## W(u); }                               \
static inline T N ## _read_from_le(const void *p)                           \
{ uint ## W ## _t u = *(uint ## W ## _t *)p;                                \
  u = le ## W ## toh(u); return (T)u; }                                     \
static inline void N ## _write_to_le(void *p, T v)                          \
{ uint ## W ## _t u = (uint ## W ## _t)v;                                   \
  *(uint ## W ## _t *)p = htole ## W(u); }                                  \
static inline T N ## _read_from_be(const void *p)                           \
{ uint ## W ## _t u = *(uint ## W ## _t *)p;                                \
  u = be ## W ## toh(u); return (T)u; }                                     \
static inline void N ## _write_to_be(void *p, T v)                          \
{ uint ## W ## _t u = (uint ## W ## _t)v;                                   \
  *(uint ## W ## _t *)p = htobe ## W(u); }                                  \
__flatcc_basic_scalar_accessors_impl(N, T, W, E)

#define __flatcc_define_real_accessors_impl(N, T, W, E)                     \
static inline T N ## _read_from_pe(const void *p)                           \
{ T v; uint ## W ## _t u = *(uint ## W ## _t *)p;                           \
  u = E ## W ## toh(u); memcpy(&v, &u, sizeof(v)); return v; }              \
static inline void N ## _write_to_pe(void *p, T v)                          \
{ uint ## W ## _t u; memcpy(&u, &v, sizeof(v));                             \
  *(uint ## W ## _t *)p = hto ## E ## W(u); }                               \
static inline T N ## _read_from_le(const void *p)                           \
{ T v; uint ## W ## _t u = *(uint ## W ## _t *)p;                           \
  u = le ## W ## toh(u); memcpy(&v, &u, sizeof(v)); return v; }             \
static inline void N ## _write_to_le(void *p, T v)                          \
{ uint ## W ## _t u; memcpy(&u, &v, sizeof(v));                             \
  *(uint ## W ## _t *)p = htole ## W(u); }                                  \
static inline T N ## _read_from_be(const void *p)                           \
{ T v; uint ## W ## _t u = *(uint ## W ## _t *)p;                           \
  u = be ## W ## toh(u); memcpy(&v, &u, sizeof(v)); return v; }             \
static inline void N ## _write_to_be(void *p, T v)                          \
{ uint ## W ## _t u; memcpy(&u, &v, sizeof(v));                             \
  *(uint ## W ## _t *)p = htobe ## W(u); }                                  \
__flatcc_basic_scalar_accessors_impl(N, T, W, E)

#define __flatcc_define_integer_accessors(N, T, W, E)                       \
__flatcc_define_integer_accessors_impl(N, T, W, E)

#define __flatcc_define_real_accessors(N, T, W, E)                          \
__flatcc_define_real_accessors_impl(N, T, W, E)

#define __flatcc_define_basic_integer_accessors(NS, TN, T, W, E)            \
__flatcc_define_integer_accessors(NS ## TN, T, W, E)

#define __flatcc_define_basic_real_accessors(NS, TN, T, W, E)               \
__flatcc_define_real_accessors(NS ## TN, T, W, E)

#define __flatcc_define_basic_scalar_accessors(NS, E)                       \
__flatcc_define_basic_integer_accessors(NS, char, char, 8, E)               \
__flatcc_define_basic_integer_accessors(NS, uint8, uint8_t, 8, E)           \
__flatcc_define_basic_integer_accessors(NS, uint16, uint16_t, 16, E)        \
__flatcc_define_basic_integer_accessors(NS, uint32, uint32_t, 32, E)        \
__flatcc_define_basic_integer_accessors(NS, uint64, uint64_t, 64, E)        \
__flatcc_define_basic_integer_accessors(NS, int8, int8_t, 8, E)             \
__flatcc_define_basic_integer_accessors(NS, int16, int16_t, 16, E)          \
__flatcc_define_basic_integer_accessors(NS, int32, int32_t, 32, E)          \
__flatcc_define_basic_integer_accessors(NS, int64, int64_t, 64, E)          \
__flatcc_define_basic_real_accessors(NS, float, float, 32, E)               \
__flatcc_define_basic_real_accessors(NS, double, double, 64, E)

#ifdef __cplusplus
}
#endif

#endif /* FLATCC_ACCESSORS */
