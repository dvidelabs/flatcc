#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "codegen_c.h"
#include "codegen_c_sort.h"
#include "fileio.h"
#include "../../external/hash/str_set.h"

#define llu(x) (long long unsigned int)(x)
#define lld(x) (long long int)(x)

/* Finds first occurrence of matching key when vector is sorted on the named field. */
static void gen_find(output_t *out)
{
    const char *nsc = out->nsc;

    /*
     * E: Element accessor (elem = E(vector, index)).
     * A: Field accessor (or the identity function), result must match the diff function D's first arg.
     * V: The vector to search (assuming sorted).
     * T: The scalar, enum or string key type, (either the element, or a field of the element).
     * K: The search key.
     * Kn: optional key length so external strings do not have to be zero terminated.
     * D: the diff function D(v, K, Kn) :: v - <K, Kn>
     *
     * returns index (0..len - 1), or not_found (-1).
     */
    fprintf(out->fp,
        "static %suoffset_t %snot_found = (%suoffset_t)-1;\n"
        "#define __%sidentity(n) (n)\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/* Subtraction doesn't work for unsigned types. */\n"
        "#define __%sscalar_cmp(x, y, n) ((x) < (y) ? -1 : (x) > (y))\n"
        "static inline int __%sstring_n_cmp(%sstring_t v, const char *s, size_t n)\n"
        "{ %suoffset_t nv = %sstring_len(v); int x = strncmp(v, s, nv < n ? nv : n);\n"
        "  return x != 0 ? x : nv < n ? -1 : nv > n; }\n"
        "/* `n` arg unused, but needed by string find macro expansion. */\n"
        "static inline int __%sstring_cmp(%sstring_t v, const char *s, size_t n) { (void)n; return strcmp(v, s); }\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/* A = identity if searching scalar vectors rather than key fields. */\n"
        "/* Returns lowest matching index not_found. */\n"
        "#define __%sfind_by_field(A, V, E, L, K, Kn, T, D)\\\n"
        "{ T v; %suoffset_t a = 0, b, m; if (!(b = L(V))) { return %snot_found; }\\\n"
        "  --b;\\\n"
        "  while (a < b) {\\\n"
        "    m = a + ((b - a) >> 1);\\\n"
        "    v = A(E(V, m));\\\n"
        "    if ((D(v, (K), (Kn))) < 0) {\\\n"
        "      a = m + 1;\\\n"
        "    } else {\\\n"
        "      b = m;\\\n"
        "    }\\\n"
        "  }\\\n"
        "  if (a == b) {\\\n"
        "    v = A(E(V, a));\\\n"
        "    if (D(v, (K), (Kn)) == 0) {\\\n"
        "       return a;\\\n"
        "    }\\\n"
        "  }\\\n"
        "  return %snot_found;\\\n"
        "}\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sfind_by_scalar_field(A, V, E, L, K, T)\\\n"
        "__%sfind_by_field(A, V, E, L, K, 0, T, __%sscalar_cmp)\n"
        "#define __%sfind_by_string_field(A, V, E, L, K)\\\n"
        "__%sfind_by_field(A, V, E, L, K, 0, %sstring_t, __%sstring_cmp)\n"
        "#define __%sfind_by_string_n_field(A, V, E, L, K, Kn)\\\n"
        "__%sfind_by_field(A, V, E, L, K, Kn, %sstring_t, __%sstring_n_cmp)\n",
        nsc, nsc, nsc, nsc, nsc,
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_find_by_scalar_field(N, NK, TK)\\\n"
        "static inline %suoffset_t N ## _vec_find_by_ ## NK(N ## _vec_t vec, TK key)\\\n"
        "__%sfind_by_scalar_field(N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, TK)\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scalar_find(N, T)\\\n"
        "static inline %suoffset_t N ## _vec_find(N ## _vec_t vec, T key)\\\n"
        "__%sfind_by_scalar_field(__%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\n",
        nsc, nsc, nsc, nsc);
}

static void gen_helpers(output_t *out)
{
    const char *nsc = out->nsc;

    fprintf(out->fp,
        /*
         * Derived from flatcc_types.h but avoids direct dependency by
         * copying here. Assumes stdint.h.
         */
        "#ifndef flatbuffers_types_defined\n"
        "#define flatbuffers_types_defined\n"
        "\n"
        "#define flatbuffers_uoffset_t_defined\n"
        "#define flatbuffers_soffset_t_defined\n"
        "#define flatbuffers_voffset_t_defined\n"
        "\n"
        "#define FLATBUFFERS_UOFFSET_MAX UINT%u_MAX\n"
        "#define FLATBUFFERS_SOFFSET_MAX INT%u_MAX\n"
        "#define FLATBUFFERS_SOFFSET_MIN INT%u_MIN\n"
        "#define FLATBUFFERS_VOFFSET_MAX UINT%u_MAX\n"
        "\n"
        "#define FLATBUFFERS_UOFFSET_WIDTH %u\n"
        "#define FLATBUFFERS_SOFFSET_WIDTH %u\n"
        "#define FLATBUFFERS_VOFFSET_WIDTH %u\n"
        "\n"
        "typedef uint%u_t flatbuffers_uoffset_t;\n"
        "typedef int%u_t flatbuffers_soffset_t;\n"
        "typedef uint%u_t flatbuffers_voffset_t;\n"
        "\n"
        "#define FLATBUFFERS_IDENTIFIER_SIZE 4\n"
        "\n"
        "typedef char flatbuffers_fid_t[FLATBUFFERS_IDENTIFIER_SIZE];\n"
        "\n"
        "#define FLATBUFFERS_ID_MAX (FLATBUFFERS_VOFFSET_MAX / sizeof(flatbuffers_voffset_t) - 3)\n"
        "#define FLATBUFFERS_COUNT_MAX(elem_size) ((elem_size) > 0 ? FLATBUFFERS_UOFFSET_MAX/(elem_size) : 0)\n"
        "\n"
        "#endif /* flatbuffers_types_defined */\n"
        "\n\n",
        out->opts->offset_size * 8,
        out->opts->offset_size * 8,
        out->opts->offset_size * 8,
        out->opts->voffset_size * 8,
        out->opts->offset_size * 8,
        out->opts->offset_size * 8,
        out->opts->voffset_size * 8,
        out->opts->offset_size * 8,
        out->opts->offset_size * 8,
        out->opts->voffset_size * 8);
    fprintf(out->fp,
        "/* Helpers to standardize type definitions.\n*/"
        "#undef le8toh\n"
        "#define le8toh(n) (n)\n"
        "#undef htole8\n"
        "#define htole8(n) (n)\n"
        "#undef be8toh\n"
        "#define be8toh(n) (n)\n"
        "#undef htobe8\n"
        "#define htobe8(n) (n)\n"
    );
    if (strcmp(nsc, "flatbuffers_")) {
        fprintf(out->fp,
                "typedef flatbuffers_uoffset_t %suoffset_t;\n"
                "typedef flatbuffers_soffset_t %ssoffset_t;\n"
                "typedef flatbuffers_voffset_t %svoffset_t;\n"
                "\n",
                nsc, nsc, nsc);
    }
    fprintf(out->fp,
        "/*\n"
        " * Define flatbuffer accessors be little endian.\n"
        " * This must match the `flatbuffers_is_native_pe` definition.\n"
        " */\n");
    fprintf(out->fp,
        "/*\n"
        " * NOTE: the strict pointer aliasing rule in C makes it non-trival to byteswap\n"
        " * floating point values using intrinsic integer based functions like `le32toh`.\n"
        " * We cannot cast through pointers as it will generate warnings and potentially\n"
        " * violate optimizer assumptions. Cast through union is a safer approach.\n"
        " */\n"
        "union __%sfu32 { float f; uint32_t u32; };\n"
        "static_assert(sizeof(union __%sfu32) == 4, \"float cast union should have size 4\");\n"
        "union __%sdu64 { double d; uint64_t u64; };\n"
        "static_assert(sizeof(union __%sdu64) == 8, \"double cast union should have size 8\");\n"
        "\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_integer_accessors(N, T, W)\\\n"
        "static inline T N ## _cast_from_pe(T v)\\\n"
        "{ return (T) le ## W ## toh((uint ## W ## _t)v); }\\\n"
        "static inline T N ## _cast_to_pe(T v)\\\n"
        "{ return (T) htole ## W((uint ## W ## _t)v); }\\\n"
        "static inline T N ## _read_from_pe(const void *p)\\\n"
        "{ return N ## _cast_from_pe(*(T *)p); }\\\n"
        "static inline T N ## _read_to_pe(const void *p)\\\n"
        "{ return N ## _cast_to_pe(*(T *)p); }\\\n"
        "static inline T N ## _read(const void *p)\\\n"
        "{ return *(T *)p; }\n",
        nsc);
    fprintf(out->fp,
        "static inline float %sfloat_cast_from_pe(float f)\n"
        "{ union __%sfu32 x; x.f = f; x.u32 = le32toh(x.u32); return x.f; }\n"
        "static inline double %sdouble_cast_from_pe(double d)\n"
        "{ union __%sdu64 x; x.d = d; x.u64 = le64toh(x.u64); return x.d; }\n"
        "static inline float %sfloat_cast_to_pe(float f)\n"
        "{ union __%sfu32 x; x.f = f; x.u32 = htole32(x.u32); return x.f; }\n"
        "static inline double %sdouble_cast_to_pe(double d)\n"
        "{ union __%sdu64 x; x.d = d; x.u64 = htole64(x.u64); return x.d; }\n"
        "static inline float %sfloat_read_from_pe(const void *p)\n"
        "{ union __%sfu32 x; x.u32 = le32toh(*(uint32_t *)p); return x.f; }\n"
        "static inline double %sdouble_read_from_pe(const void *p)\n"
        "{ union __%sdu64 x; x.u64 = le64toh(*(uint64_t *)p); return x.d; }\n"
        "static inline float %sfloat_read_to_pe(const void *p)\n"
        "{ union __%sfu32 x; x.u32 = htole32(*(uint32_t *)p); return x.f; }\n"
        "static inline double %sdouble_read_to_pe(const void *p)\n"
        "{ union __%sdu64 x; x.u64 = htole64(*(uint64_t *)p); return x.d; }\n"
        "static inline float %sfloat_read(const void *p) { return *(float *)p; }\n"
        "static inline double %sdouble_read(const void *p) { return *(double *)p; }\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
        nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/* This allows us to use FLATBUFFERS_UOFFSET_WIDTH for W. */\n"
        "#define __%sPASTE_3(A, B, C) A ## B ## C\n"
        "#define __%sLOAD_PE(W, n) __%sPASTE_3(le, W, toh)(n)\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define %sload_uoffset(n)\\\n"
        "(flatbuffers_uoffset_t)__%sLOAD_PE(FLATBUFFERS_UOFFSET_WIDTH, n)\n"
        "#define %sload_soffset(n)\\\n"
        "(flatbuffers_soffset_t) __%sLOAD_PE(FLATBUFFERS_SOFFSET_WIDTH, n)\n"
        "#define %sload_voffset(n)\\\n"
        "(flatbuffers_voffset_t) __%sLOAD_PE(FLATBUFFERS_VOFFSET_WIDTH, n)\n",
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/****************************************************************\n"
        " *  From this point on endianness is abstracted away accessors. *\n"
        " *  Protocol endian (pe) may be either little or big endian.    *\n"
        " ****************************************************************/\n");
    fprintf(out->fp,
            "typedef uint8_t %sbool_t;\n"
            "static const %sbool_t %strue = 1;\n"
            "static const %sbool_t %sfalse = 0;\n",
            nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sread_scalar_at_byteoffset(N, p, o) N ## _read_from_pe((uint8_t *)(p) + (o))\n"
        "#define __%sread_scalar(N, p) N ## _read_from_pe(p)\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%sread_vt(ID, offset, t)\\\n"
        "%svoffset_t offset = 0;\\\n"
        "{\\\n"
        "    assert(t != 0 && \"null pointer table access\");\\\n"
        "    %svoffset_t id = ID;\\\n"
        "    %svoffset_t *vt = (%svoffset_t *)((uint8_t *)(t) -\\\n"
        "        (%ssoffset_t)(%sload_uoffset(*(%suoffset_t *)(t))));\\\n"
        "    if (%sload_voffset(vt[0]) >= sizeof(vt[0]) * (id + 3)) {\\\n"
        "        offset = %sload_voffset(vt[id + 2]);\\\n"
        "    }\\\n"
        "}\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sfield_present(ID, t) { __%sread_vt(ID, offset, t) return offset != 0; }\n",
            nsc, nsc);
    fprintf(out->fp,
        "#define __%sscalar_field(N, ID, V, t)\\\n"
        "{\\\n"
        "    __%sread_vt(ID, offset, t)\\\n"
        "    return offset ? __%sread_scalar_at_byteoffset(N, t, offset) : V;\\\n"
        "}\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sstruct_field(T, ID, t, r)\\\n"
        "{\\\n"
        "    __%sread_vt(ID, offset, t)\\\n"
        "    if (offset) {\\\n"
        "        return (T)((uint8_t *)(t) + offset);\\\n"
        "    }\\\n"
        "    assert(!(r) && \"required field missing\");\\\n"
        "    return 0;\\\n"
        "}\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%soffset_field(T, ID, t, r, adjust)\\\n"
        "{\\\n"
        "    %suoffset_t *elem;\\\n"
        "    __%sread_vt(ID, offset, t)\\\n"
        "    if (offset) {\\\n"
        "        elem = (%suoffset_t *)((uint8_t *)(t) + offset);\\\n"
        "        /* Add sizeof so C api can have raw access past header field. */\\\n"
        "        return (T)((uint8_t *)(elem) + adjust +\\\n"
        "            (%suoffset_t)((%sload_uoffset(*elem))));\\\n"
        "    }\\\n"
        "    assert(!(r) && \"required field missing\");\\\n"
        "    return 0;\\\n"
        "}\n",
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%svector_field(T, ID, t, r) __%soffset_field(T, ID, t, r, sizeof(%suoffset_t))\n"
        "#define __%stable_field(T, ID, t, r) __%soffset_field(T, ID, t, r, 0)\n",
        nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%svec_len(vec)\\\n"
        "{ return (vec) ? %sload_uoffset((((%suoffset_t *)(vec))[-1])) : 0; }\n"
        "#define __%sstring_len(s) __%svec_len(s)\n",
        nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "static inline %suoffset_t %svec_len(const void *vec)\n"
        "__%svec_len(vec)\n",
        nsc, nsc,
        nsc);
    fprintf(out->fp,
        /* Tb is the base type for loads. */
        "#define __%sscalar_vec_at(N, vec, i)\\\n"
        "{ assert(%svec_len(vec) > (i) && \"index out of range\");\\\n"
        "  return __%sread_scalar(N, &(vec)[i]); }\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sstruct_vec_at(vec, i)\\\n"
        "{ assert(%svec_len(vec) > (i) && \"index out of range\"); return (vec) + (i); }\n",
        nsc, nsc);
    fprintf(out->fp,
        "/* `adjust` skips past the header for string vectors. */\n"
        "#define __%soffset_vec_at(T, vec, i, adjust)\\\n"
        "{ assert(%svec_len(vec) > (i) && \"index out of range\");\\\n"
        "  const %suoffset_t *elem = (vec) + (i);\\\n"
        "  return (T)((uint8_t *)(elem) + %sload_uoffset(*elem) + adjust); }\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sdefine_scalar_vec_len(N) \\\n"
            "static inline %suoffset_t N ## _vec_len(N ##_vec_t vec)\\\n"
            "{ return %svec_len(vec); }\n",
            nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sdefine_scalar_vec_at(N, T) \\\n"
            "static inline T N ## _vec_at(N ## _vec_t vec, %suoffset_t i)\\\n"
            "__%sscalar_vec_at(N, vec, i)\n",
            nsc, nsc, nsc);
    fprintf(out->fp,
            "typedef const char *%sstring_t;\n"
            "static inline %suoffset_t %sstring_len(%sstring_t s)\n"
            "__%sstring_len(s)\n",
            nsc,
            nsc, nsc, nsc,
            nsc);
    fprintf(out->fp,
            "typedef const %suoffset_t *%sstring_vec_t;\n"
            "typedef %suoffset_t *%sstring_mutable_vec_t;\n"
            "static inline %suoffset_t %sstring_vec_len(%sstring_vec_t vec)\n"
            "__%svec_len(vec)\n"
            "static inline %sstring_t %sstring_vec_at(%sstring_vec_t vec, %suoffset_t i)\n"
            "__%soffset_vec_at(%sstring_t, vec, i, sizeof(vec[0]))\n",
            nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "typedef const void *%sgeneric_table_t;\n",
            nsc);
    gen_find(out);
    if (out->opts->cgen_sort) {
        gen_sort(out);
    } else {
        fprintf(out->fp, "/* sort disabled */\n");
    }
    fprintf(out->fp,
            "#define __%sdefine_scalar_vector(N, T, W)\\\n"
            "typedef const T *N ## _vec_t;\\\n"
            "typedef T *N ## _mutable_vec_t;\\\n"
            "__%sdefine_scalar_vec_len(N)\\\n"
            "__%sdefine_scalar_vec_at(N, T)\\\n"
            "__%sdefine_scalar_find(N, T)\\\n",
            nsc, nsc, nsc, nsc);
    if (out->opts->cgen_sort) {
        fprintf(out->fp, "\\\n__%sdefine_scalar_sort(N, T)\n", nsc);
    }
    fprintf(out->fp, "\n");
    fprintf(out->fp,
            "#define __%sdefine_integer_type(N, T, W)\\\n"
            "__%sdefine_integer_accessors(N, T, W)\\\n"
            "__%sdefine_scalar_vector(N, T, W)\n"
            "#define __%sdefine_real_type(N, T, W)\\\n"
            "/* `double` and `float` scalars accessors defined. */\\\n"
            "__%sdefine_scalar_vector(N, T, W)\\\n"
            "\n",
            nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "__%sdefine_integer_type(%sbool, %sbool_t, 8)\n"
            "__%sdefine_integer_type(%suint8, uint8_t, 8)\n"
            "__%sdefine_integer_type(%sint8, int8_t, 8)\n"
            "__%sdefine_integer_type(%suint16, uint16_t, 16)\n"
            "__%sdefine_integer_type(%sint16, int16_t, 16)\n"
            "__%sdefine_integer_type(%suint32, uint32_t, 32)\n"
            "__%sdefine_integer_type(%sint32, int32_t, 32)\n"
            "__%sdefine_integer_type(%suint64, uint64_t, 64)\n"
            "__%sdefine_integer_type(%sint64, int64_t, 64)\n"
            "__%sdefine_real_type(%sfloat, float, 32)\n"
            "__%sdefine_real_type(%sdouble, double, 64)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "static inline %suoffset_t %sstring_vec_find(%sstring_vec_t vec, const char *s)\n"
            "__%sfind_by_string_field(__%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline %suoffset_t %sstring_vec_find_n(%sstring_vec_t vec, const char *s, size_t n)\n"
            "__%sfind_by_string_n_field(__%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    if (out->opts->cgen_sort) {
        fprintf(out->fp, "__%sdefine_string_sort()\n", nsc);
    }
    fprintf(out->fp,
            "#define __%sstruct_scalar_field(t, M, N)\\\n"
            "{ return t ? __%sread_scalar(N, &(t->M)) : 0; }\n"
            "#define __%sstruct_struct_field(t, M) { return t ? &(t->M) : 0; }\n",
            nsc, nsc, nsc);
    fprintf(out->fp,
            "/* If fid is null, the function returns true without testing as buffer is not expected to have any id. */\n"
            "static inline int %shas_identifier(const void *buffer, const char *fid)\n"
            "{ return fid == 0 || (%sload_uoffset(((%suoffset_t *)buffer)[1]) ==\n"
            "      ((%suoffset_t)fid[0]) + (((%suoffset_t)fid[1]) << 8) +\n"
            "      (((%suoffset_t)fid[2]) << 16) + (((%suoffset_t)fid[3]) << 24)); }\n"
            "#define %sverify_endian() %shas_identifier(\"\\x00\\x00\\x00\\x00\" \"1234\", \"1234\")\n",
            nsc,
            nsc, nsc, nsc, nsc,
            nsc, nsc,
            nsc, nsc);

    fprintf(out->fp,
            "/* Null file identifier accepts anything, otherwise fid should be 4 characters. */\n"
            "#define __%sread_root(T, K, buffer, fid)\\\n"
            "  ((!buffer || !%shas_identifier(buffer, fid)) ? 0 :\\\n"
            "  ((T ## _ ## K ## t)(((uint8_t *)buffer) +\\\n"
            "  %sload_uoffset(((%suoffset_t *)buffer)[0]))))\n",
            nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%snested_buffer_as_root(C, N, T, K)\\\n"
            "static inline T ## _ ## K ## t C ## _ ## N ## _as_root_with_identifier(C ## _ ## table_t t, const char *fid)\\\n"
            "{ const uint8_t *buffer = C ## _ ## N(t); return __%sread_root(T, K, buffer, fid); }\\\n"
            "static inline T ## _ ## K ## t C ## _ ## N ## _as_root(C ## _ ## table_t t)\\\n"
            "{ const char *fid = T ## _identifier;\\\n"
            "  const uint8_t *buffer = C ## _ ## N(t); return __%sread_root(T, K, buffer, fid); }\n",
            nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sbuffer_as_root(N, K)\\\n"
            "static inline N ## _ ## K ## t N ## _as_root_with_identifier(const void *buffer, const char *fid)\\\n"
            "{ return __%sread_root(N, K, buffer, fid); }\\\n"
            "static inline N ## _ ## K ## t N ## _as_root(const void *buffer)\\\n"
            "{ const char *fid = N ## _identifier;\\\n"
            "  return __%sread_root(N, K, buffer, fid); }\n"
            "#define __%sstruct_as_root(N) __%sbuffer_as_root(N, struct_)\n"
            "#define __%stable_as_root(N) __%sbuffer_as_root(N, table_)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp, "\n");
}

int fb_gen_common_c_header(output_t *out)
{
    const char *nscup = out->nscup;

    fprintf(out->fp,
        "#ifndef %s_COMMON_READER_H\n"
        "#define %s_COMMON_READER_H\n",
        nscup, nscup);
    fprintf(out->fp, "\n/* " FLATCC_GENERATED_BY " */\n\n");
    fprintf(out->fp, "/* Common FlatBuffers read functionality for C. */\n\n");
    if (!out->opts->cgen_sort) {
        fprintf(out->fp,
                "/*"
                " * This code is generated without support for vector sort operations\n"
                " * but find operations are supported on pre-sorted vectors.\n"
                " */\n");
    }
    gen_pragma_push(out);
    fprintf(out->fp,
            "/* Include portability layer first if not on compliant C11 platform. */\n"
            "#ifdef FLATCC_PORTABLE\n"
            "#include \"flatcc/flatcc_portable.h\"\n"
            "#endif\n"
            "#ifndef UINT8_MAX\n"
            "#include <stdint.h>\n"
            "#endif\n");
    fprintf(out->fp,
            "/*\n"
            " * For undetected platforms, provide a custom <endian.h> file in the include path,\n"
            " * or include equivalent functionality (the 6 lexxtoh, htolexx functions) before this file.\n"
            " */\n"
            "#if !defined(FLATBUFFERS_LITTLEENDIAN)\n"
            "#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) ||\\\n"
            "      (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||\\\n"
            "      (defined(_BYTE_ORDER) && _BYTE_ORDER == _LITTLE_ENDIAN) ||\\\n"
            "    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) ||\\\n"
            "    /* for MSVC */ defined(_M_X64) || defined(_M_IX86) || defined(_M_I86) ||\\\n"
            "    defined(__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)\n"
            "#define FLATBUFFERS_LITTLEENDIAN 1\n"
            "#elif (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) ||\\\n"
            "      (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) ||\\\n"
            "      (defined(_BYTE_ORDER) && _BYTE_ORDER == _BIG_ENDIAN) ||\\\n"
            "    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) ||\\\n"
            "    defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)\n"
            "#define FLATBUFFERS_LITTLEENDIAN 0\n"
            "#endif\n"
            "#endif\n"
            "#if defined(flatbuffers_is_native_pe)\n"
            "/* NOP */\n"
            "#elif !defined(le16toh) && FLATBUFFERS_LITTLEENDIAN\n"
            "#define le16toh(n) ((uint16_t)(n))\n"
            "#define le32toh(n) ((uint32_t)(n))\n"
            "#define le64toh(n) ((uint64_t)(n))\n"
            "#define htole16(n) ((uint16_t)(n))\n"
            "#define htole32(n) ((uint32_t)(n))\n"
            "#define htole64(n) ((uint64_t)(n))\n"
            "/* pe indicates protocol endian, which for FlatBuffers is little endian. */\n"
            "#define flatbuffers_is_native_pe() 1\n"
            "#else\n"
            "#if defined(OS_FREEBSD) || defined(OS_OPENBSD) || defined(OS_NETBSD) || defined(OS_DRAGONFLYBSD)\n"
            "#include <sys/types.h>\n"
            "#include <sys/endian.h>\n"
            "#else\n"
            "#include <endian.h>\n"
            "#endif\n"
            "#if defined(letoh16) && !defined(le16toh)\n"
            "#define le16toh letoh16\n"
            "#define le32toh letoh32\n"
            "#define le64toh letoh64\n"
            "#endif\n"
            "#endif\n"
            "#ifndef flatbuffers_is_native_pe\n"
            "#ifdef le16toh\n"
            "/* This will not automatically define FLATBUFFERS_LITTLEENDIAN, but it will work. */\n"
            "#define flatbuffers_is_native_pe() (le16toh(1) == 1)\n"
            "#else\n"
            "#error \"unable to detect endianness - define FLATBUFFERS_LITTLEENDIAN to 1 or 0\"\n"
            "#endif\n"
            "#endif\n"
            "#include <assert.h>\n"
            "/* clang assert.h sometimes fail to do this even with -std=c11 */\n"
            "#ifndef static_assert\n"
            "#ifndef FLATBUFFERS_NO_STATIC_ASSERT\n"
            "#define static_assert(pred, msg) _Static_assert(pred, msg)\n"
            "#else\n"
            "#define static_assert(pred, msg)\n"
            "#endif\n"
            "#endif\n"
            "/* Only needed for string search. */\n"
            "#include <string.h>\n"
            "\n");
    gen_helpers(out);
    gen_pragma_pop(out);
    fprintf(out->fp,
        "#endif /* %s_COMMON_H */\n",
        nscup);
    return 0;
}

static void _str_set_destructor(void *context, char *item)
{
    (void)context;

    free(item);
}

/*
 * Removal of duplicate inclusions is only for a cleaner output - it is
 * not stricly necessary because the preprocessor handles include
 * guards. The guards are required to deal with concatenated files
 * regardless unless we generate special code for concatenation.
 */
void fb_gen_c_includes(output_t *out, const char *ext, const char *extup)
{
    fb_include_t *inc = out->S->includes;
    char *basename, *basenameup, *s;
    str_set_t set;

    fb_clear(set);

    /* Don't include our own file. */
    str_set_insert_item(&set, fb_copy_path(out->S->basenameup, -1), ht_keep);
    while (inc) {
        checkmem((basename = fb_create_basename(
                    inc->name.s.s, inc->name.s.len, out->opts->default_schema_ext)));
        inc = inc->link;
        checkmem((basenameup = fb_copy_path(basename, -1)));
        s = basenameup;
        while (*s) {
            *s = toupper(*s);
            ++s;
        }
        if (str_set_insert_item(&set, basenameup, ht_keep)) {
            free(basenameup);
            free(basename);
            continue;
        }
        /* The include guard is needed when concatening output. */
        fprintf(out->fp,
            "#ifndef %s%s\n"
            "#include \"%s%s\"\n"
            "#endif\n",
            basenameup, extup, basename, ext);
        free(basename);
        /* `basenameup` stored in str_set. */
    }
    str_set_destroy(&set, _str_set_destructor, 0);
}

static void gen_pretext(output_t *out)
{
    const char *nsc = out->nsc;
    const char *nscup = out->nscup;

    int do_pad = out->opts->cgen_pad;

    fprintf(out->fp,
        "#ifndef %s_READER_H\n"
        "#define %s_READER_H\n",
        out->S->basenameup, out->S->basenameup);

    fprintf(out->fp, "\n/* " FLATCC_GENERATED_BY " */\n\n");
    if (do_pad) {
        fprintf(out->fp,
        "/*\n"
        " * Generated with 'pad' option which expects #pragma pack(1) and\n"
        " * #pragma pack() to be supported, and which adds extra padding\n"
        " * fields to structs.\n"
        " *\n"
        " * This is mostly relevant for some micro controller platforms, but\n"
        " * may also be needed with 'force_align' attributes > 16.\n"
        " *\n"
        " * The default output uses C11 <stdalign.h> alignas(n) which can be\n"
        " * defined as `__attribute__((aligned (n)))` or similar on many\n"
        " * older platforms.\n"
        " */\n"
        "\n");
    }

    fprintf(out->fp,
            "#ifndef %s_COMMON_READER_H\n"
            "#include \"%scommon_reader.h\"\n"
            "#endif\n",
            nscup, nsc);
    fb_gen_c_includes(out, "_reader.h", "_READER_H");

    if (!do_pad) {
        fprintf(out->fp,
                "#ifndef alignas\n"
                "#include <stdalign.h>\n"
                "#endif\n");
    }
    gen_pragma_push(out);
    if (out->S->file_identifier.type == vt_string) {
        fprintf(out->fp,
            "#undef %sidentifier\n"
            "#define %sidentifier \"%.*s\"\n",
            nsc,
            nsc, out->S->file_identifier.s.len, out->S->file_identifier.s.s);
    } else {
        fprintf(out->fp,
            "#ifndef %sidentifier\n"
            "#define %sidentifier 0\n"
            "#endif\n",
            nsc, nsc);
    }
    if (out->S->file_extension.type == vt_string) {
        fprintf(out->fp,
            "#undef %sextension\n"
            "#define %sextension \".%.*s\"\n",
            nsc,
            nsc, out->S->file_extension.s.len, out->S->file_extension.s.s);
    } else {
        fprintf(out->fp,
            /* Configured extensions include dot, schema does not. */
            "#ifndef %sextension\n"
            "#define %sextension \"%s\"\n"
            "#endif\n",
            nsc, nsc, out->opts->default_bin_ext);
    }
    fprintf(out->fp,
            "#ifndef %s_WRAP_NAMESPACE\n"
            "#define %s_WRAP_NAMESPACE(ns, x) ns ## _ ## x\n"
            "#endif\n",
            nscup, nscup);
    fprintf(out->fp, "\n");
}

static void gen_footer(output_t *out)
{
    gen_pragma_pop(out);
    fprintf(out->fp, "#endif /* %s_H */\n", out->S->basenameup);
}

static void gen_forward_decl(output_t *out, fb_compound_type_t *ct)
{
    fb_scoped_name_t snt;
    const char *nsc = out->nsc;

    fb_clear(snt);

    assert(ct->symbol.kind == fb_is_struct || ct->symbol.kind == fb_is_table);

    fb_compound_name(ct, &snt);
    if (ct->symbol.kind == fb_is_struct) {
        fprintf(out->fp, "typedef struct %s %s_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef const struct %s *%s_struct_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef struct %s *%s_mutable_struct_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef const struct %s *%s_vec_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef struct %s *%s_mutable_vec_t;\n",
                snt.text, snt.text);
    } else {
        fprintf(out->fp, "typedef const struct %s_table *%s_table_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef const %suoffset_t *%s_vec_t;\n", nsc, snt.text);
        fprintf(out->fp, "typedef %suoffset_t *%s_mutable_vec_t;\n", nsc, snt.text);
    }
}

static const char *scalar_vector_type_name(fb_scalar_type_t scalar_type)
{
    const char *tname;
    switch (scalar_type) {
    case fb_ulong:
        tname = "uint64_vec_t";
        break;
    case fb_uint:
        tname = "uint32_vec_t";
        break;
    case fb_ushort:
        tname = "uint16_vec_t";
        break;
    case fb_ubyte:
        tname = "uint8_vec_t";
        break;
    case fb_bool:
        tname = "uint8_vec_t";
        break;
    case fb_long:
        tname = "int64_vec_t";
        break;
    case fb_int:
        tname = "int32_vec_t";
        break;
    case fb_short:
        tname = "int16_vec_t";
        break;
    case fb_byte:
        tname = "bool_vec_t";
        break;
    case fb_float:
        tname = "float_vec_t";
        break;
    case fb_double:
        tname = "double_vec_t";
        break;
    default:
        gen_panic(out, "internal error: unexpected type during code generation");
        tname = __FLATCC_ERROR_TYPE;
        break;
    }
    return tname;
}

static const char *scalar_suffix(fb_scalar_type_t scalar_type)
{
    const char *suffix;
    switch (scalar_type) {
    case fb_ulong:
        suffix = "ULL";
        break;
    case fb_uint:
        suffix = "UL";
        break;
    case fb_ushort:
        suffix = "U";
        break;
    case fb_ubyte:
        suffix = "U";
        break;
    case fb_bool:
        suffix = "U";
        break;
    case fb_long:
        suffix = "LL";
        break;
    case fb_int:
        suffix = "L";
        break;
    case fb_short:
        suffix = "";
        break;
    case fb_byte:
        suffix = "";
        break;
    case fb_double:
        suffix = "";
    case fb_float:
        suffix = "F";
    default:
        gen_panic(out, "internal error: unexpected type during code generation");
        suffix = "";
        break;
    }
    return suffix;
}

void fb_scoped_symbol_name(fb_scope_t *scope, fb_symbol_t *sym, fb_scoped_name_t *sn)
{
    fb_token_t *t = sym->ident;

    if (sn->scope != scope) {
        if (0 > (sn->scope_len = fb_copy_scope(scope, sn->text))) {
            sn->scope_len = 0;
            fprintf(stderr, "skipping too long namespace\n");
        }
    }
    sn->len = t->len;
    sn->total_len = sn->scope_len + sn->len;
    if (sn->total_len > FLATCC_NAME_BUFSIZ - 1) {
        fprintf(stderr, "warning: truncating identifier: %.*s\n", sn->len, t->text);
        sn->len = FLATCC_NAME_BUFSIZ - sn->scope_len - 1;
        sn->total_len = sn->scope_len + sn->len;
    }
    memcpy(sn->text + sn->scope_len, t->text, sn->len);
    sn->text[sn->total_len] = '\0';
}

static inline void print_doc(output_t *out, const char *indent, fb_doc_t *doc)
{
    long ln = 0;
    int first = 1;
    if (doc == 0) {
        return;
    }
    while (doc) {
        if (ln != doc->ident->linenum) {
            if (first) {
                /* Not all C compilers understand // comments. */
                fprintf(out->fp, "%s/**", indent);
                ln = doc->ident->linenum;
            } else {
                fprintf(out->fp, "\n%s * ", indent);
            }
        }
        first = 0;
        fprintf(out->fp, "%.*s", (int)doc->ident->len, doc->ident->text);
        ln = doc->ident->linenum;
        doc = doc->link;
    }
    fprintf(out->fp, " */\n");
}

static void gen_struct(output_t *out, fb_compound_type_t *ct)
{
    fb_member_t *member;
    fb_symbol_t *sym;
    unsigned align;
    size_t offset = 0;
    const char *tname, *tname_ns, *tname_prefix;
    int n;
    const char *s;
    unsigned pad_index = 0, deprecated_index = 0, pad;;
    const char *kind;
    int do_pad = out->opts->cgen_pad;
    int current_key_processed, already_has_key;
    const char *nsc = out->nsc;

    fb_scoped_name_t snt;
    fb_scoped_name_t snref;

    fb_clear(snt);
    fb_clear(snref);

    assert(ct->symbol.kind == fb_is_struct);
    assert(ct->align > 0 || ct->count == 0);
    assert(ct->size > 0 || ct->count == 0);

    fb_compound_name(ct, &snt);
    print_doc(out, "", ct->doc);
    if (do_pad) {
        fprintf(out->fp, "#pragma pack(1)\n");
    }
    /*
     * Unfortunately the following is not valid in C11:
     *
     *      struct alignas(4) mystruct { ... };
     *
     * we can only use alignas on members (unlike C++, and unlike
     * non-portable C compiler variants).
     *
     * By padding the first element to the struct size we get around
     * this problem. It shouldn't strictly be necessary to add padding
     * fields, but compilers might not support padding above 16 bytes,
     * so we do that as a precaution with an optional compiler flag.
     *
     * It is unclear how to align empty structs without padding but it
     * shouldn't really matter since not field is accessed then.
     */
    fprintf(out->fp, "struct %s {\n", snt.text);
    already_has_key = 0;
    for (sym = ct->members; sym; sym = sym->link) {
        current_key_processed = 0;
        member = (fb_member_t *)sym;
        print_doc(out, "    ", member->doc);
        symbol_name(sym, &n, &s);
        align = offset == 0 ? ct->align : member->align;
        if (do_pad && (pad = member->offset - offset)) {
            fprintf(out->fp, "    uint8_t __padding%u[%u];\n",
                    pad_index++, pad);
        }
        if (member->metadata_flags & fb_f_deprecated) {
            pad = member->size;
            if (do_pad) {
                fprintf(out->fp, "    uint8_t __deprecated%u[%u]; /* was: '%.*s' */\n",
                        deprecated_index++, pad, n, s);
            } else {
                fprintf(out->fp, "    alignas(%u) uint8_t __deprecated%u[%u]; /* was: '%.*s' */\n",
                        align, deprecated_index++, pad, n, s);
            }
            offset = member->offset + member->size;
            continue;
        }
        switch (member->type.type) {
        case vt_scalar_type:
            tname_ns = scalar_type_ns(member->type.st, nsc);
            tname = scalar_type_name(member->type.st);
            if (do_pad) {
                fprintf(out->fp, "    %s%s ", tname_ns, tname);
            } else {
                fprintf(out->fp, "    alignas(%u) %s%s ", align, tname_ns, tname);
            }
            break;
        case vt_compound_type_ref:
            assert(member->type.ct->symbol.kind == fb_is_struct || member->type.ct->symbol.kind == fb_is_enum);
            kind = member->type.ct->symbol.kind == fb_is_struct ? "" : "enum_";
            fb_compound_name(member->type.ct, &snref);
            if (do_pad) {
                fprintf(out->fp, "    %s_%st ", snref.text, kind);
            } else {
                fprintf(out->fp, "    alignas(%u) %s_%st ", align, snref.text, kind);
            }
            break;
        default:
            fprintf(out->fp, "    %s ", __FLATCC_ERROR_TYPE);
            gen_panic(out, "internal error: unexpected type during code generation");
            break;
        }
        fprintf(out->fp, "%.*s;\n", n, s);
        offset = member->offset + member->size;
    }
    if (do_pad && (pad = ct->size - offset)) {
        fprintf(out->fp, "    uint8_t __padding%u[%u];\n",
                pad_index, pad);
    }
    fprintf(out->fp, "};\n");
    if (do_pad) {
        fprintf(out->fp, "#pragma pack()\n");
    }
    fprintf(out->fp,
            "static_assert(sizeof(%s_t) == %llu, \"struct size mismatch\");\n\n",
            snt.text, llu(ct->size));
    fprintf(out->fp,
            "#ifndef %s_identifier\n"
            "#define %s_identifier %sidentifier\n"
            "#endif\n",
            snt.text, snt.text, nsc);
    fprintf(out->fp,
            "static inline %suoffset_t %s_vec_len(%s_vec_t vec)\n"
            "__%svec_len(vec)\n",
            nsc, snt.text, snt.text,
            nsc);
    fprintf(out->fp,
            "static inline %s_struct_t %s_vec_at(%s_vec_t vec, %suoffset_t i)\n"
            "__%sstruct_vec_at(vec, i)\n",
            snt.text, snt.text, snt.text, nsc,
            nsc);
    fprintf(out->fp,
            "__%sstruct_as_root(%s)\n",
            nsc, snt.text);
    fprintf(out->fp, "\n");

    /* Create accessors which respect endianness and which return 0 on null struct access. */
    for (sym = ct->members; sym; sym = sym->link) {
        member = (fb_member_t *)sym;
        if (member->metadata_flags & fb_f_deprecated) {
            continue;
        }
        symbol_name(&member->symbol, &n, &s);
        switch (member->type.type) {
        case vt_scalar_type:
            tname_ns = scalar_type_ns(member->type.st, nsc);
            tname = scalar_type_name(member->type.st);
            tname_prefix = scalar_type_prefix(member->type.st);
            fprintf(out->fp,
                "static inline %s%s %s_%.*s(%s_struct_t t)\n"
                "__%sstruct_scalar_field(t, %.*s, %s%s);\n",
                tname_ns, tname, snt.text, n, s, snt.text,
                nsc, n, s, nsc, tname_prefix);
            if (member->metadata_flags & fb_f_key) {
                if (already_has_key) {
                    fprintf(out->fp, "/* Note: this is not the first field with a key on this struct. */\n");
                }
                fprintf(out->fp,     "/* Note: find only works on vectors sorted by this field. */\n");
                fprintf(out->fp,
                        "__%sdefine_find_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
                if (out->opts->cgen_sort) {
                    fprintf(out->fp,
                        "__%sdefine_sort_by_scalar_field(%s, %.*s, %s%s, %s_t)\n",
                        nsc, snt.text, n, s, tname_ns, tname, snt.text);
                }
                if (!already_has_key) {
                    fprintf(out->fp,
                        "#define %s_vec_find %s_vec_find_by_%.*s\n",
                        snt.text, snt.text, n, s);
                    if (out->opts->cgen_sort) {
                        fprintf(out->fp,
                            "#define %s_vec_sort %s_vec_sort_by_%.*s\n",
                            snt.text, snt.text, n, s);
                    }
                    already_has_key = 1;
                }
                current_key_processed = 1;
            }
            break;
        case vt_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_enum:
                tname_prefix = scalar_type_prefix(member->type.ct->type.st);
                fprintf(out->fp,
                    "static inline %s_enum_t %s_%.*s(%s_struct_t t)\n"
                    "__%sstruct_scalar_field(t, %.*s, %s%s);\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, n, s, nsc, tname_prefix);
                if (member->metadata_flags & fb_f_key) {
                    if (already_has_key) {
                        fprintf(out->fp, "/* Note: this is not the first field with a key on this table. */\n");
                    }
                    fprintf(out->fp,     "/* Note: find only works on vectors sorted by this field. */\n");
                    fprintf(out->fp,
                            "__%sdefine_find_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
                    if (out->opts->cgen_sort) {
                        fprintf(out->fp,
                            "__%sdefine_sort_by_scalar_field(%s, %.*s, %s_enum_t, %s_t)\n",
                            nsc, snt.text, n, s, snref.text, snt.text);
                    }
                    if (!already_has_key) {
                        fprintf(out->fp,
                            "#define %s_vec_find %s_vec_find_by_%.*s\n",
                            snt.text, snt.text, n, s);
                        if (out->opts->cgen_sort) {
                            fprintf(out->fp,
                                "#define %s_vec_sort %s_vec_sort_by_%.*s\n",
                                snt.text, snt.text, n, s);
                        }
                        already_has_key = 1;
                    }
                    current_key_processed = 1;
                }
                break;
            case fb_is_struct:
                /*
                 * For completeness provide an accessor which returns member pointer
                 * or null if container struct is null.
                 */
                fprintf(out->fp,
                    "static inline %s_struct_t %s_%.*s(%s_struct_t t)\n"
                    "__%sstruct_struct_field(t, %.*s);\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, n, s);
                break;
            }
        }
        if ((member->metadata_flags & fb_f_key) && !current_key_processed) {
            fprintf(out->fp,
                "/* Note: field has key, but there is no support for find by fields of this type. */\n");
            /*
             * If the first key already exists, but was for an unsupported
             * type, we do not map the next possible key to generic find.
             */
            already_has_key = 1;
        }
        fprintf(out->fp, "\n");
    }
}

/*
 * Enums are integers, but we cannot control the size.
 * To produce a typesafe and portable result, we generate constants
 * instead.
 */
static void gen_enum(output_t *out, fb_compound_type_t *ct)
{
    fb_member_t *member;
    fb_symbol_t *sym;
    const char *tname, *tname_ns, *suffix, *s, *kind;
    int n, w;
    int is_union;
    fb_scoped_name_t snt;
    const char *nsc = out->nsc;

    fb_clear(snt);

    assert(ct->symbol.kind == fb_is_enum || ct->symbol.kind == fb_is_union);
    assert(ct->type.type == vt_scalar_type);

    tname_ns = scalar_type_ns(ct->type.st, nsc);
    tname = scalar_type_name(ct->type.st);
    suffix = scalar_suffix(ct->type.st);

    w = ct->size * 8;

    is_union = ct->symbol.kind != fb_is_enum;
    kind = is_union ? "union_type" : "enum";
    fb_compound_name(ct, &snt);
    print_doc(out, "", ct->doc);
    fprintf(out->fp,
            "typedef %s%s %s_%s_t;\n",
            tname_ns, tname, snt.text, kind);
        fprintf(out->fp,
                "__%sdefine_integer_type(%s, %s_%s_t, %u)\n",
                nsc, snt.text, snt.text, kind, w);
    for (sym = ct->members; sym; sym = sym->link) {
        member = (fb_member_t *)sym;
        print_doc(out, "", member->doc);
        symbol_name(&member->symbol, &n, &s);
        switch (member->value.type) {
        case vt_uint:
            fprintf(out->fp,
                    "static const %s_%s_t %s_%.*s = %llu%s;\n",
                    snt.text, kind, snt.text, n, s, llu(member->value.u), suffix);
            break;
        case vt_int:
            fprintf(out->fp,
                    "static const %s_%s_t %s_%.*s = %lld%s;\n",
                    snt.text, kind, snt.text, n, s, lld(member->value.i), suffix);
            break;
        case vt_bool:
            fprintf(out->fp,
                    "static const %s_%s_t %s_%.*s = %u;\n",
                    snt.text, kind, snt.text, n, s, member->value.b);
            break;
        default:
            gen_panic(out, "internal error: unexpected value type in enum");
            break;
        }
    }
    fprintf(out->fp, "\n");

    if (is_union) {
        fprintf(out->fp, "static inline const char *%s_type_name(%s_union_type_t type)\n"
                "{\n",
                snt.text, snt.text);
    } else {
        fprintf(out->fp, "static inline const char *%s_name(%s_enum_t value)\n"
                "{\n",
                snt.text, snt.text);
    }

    if (is_union) {
        fprintf(out->fp, "    switch (type) {\n");
    } else {
        fprintf(out->fp, "    switch (value) {\n");
    }
    for (sym = ct->members; sym; sym = sym->link) {
        member = (fb_member_t *)sym;
        symbol_name(&member->symbol, &n, &s);
        if (sym->flags & fb_duplicate) {
            /*
             * Won't happen with ascending enums, but we can enable
             * these with a flag.
             */
            switch (member->value.type) {
            case vt_uint:
                fprintf(out->fp, "    /* case %llu: return \"%.*s\"; (duplicate) */\n",
                        llu(member->value.u), n, s);
                continue;
            case vt_int:
                fprintf(out->fp, "    /* case %lld: return \"%.*s\"; (duplicate) */\n",
                        lld(member->value.i), n, s);
                continue;
            case vt_bool:
                fprintf(out->fp, "    /* case %u: return \"%.*s\"; (duplicate) */\n",
                        member->value.b, n, s);
                continue;
            default:
                continue;
            }
        }
        switch (member->value.type) {
        case vt_uint:
            fprintf(out->fp,
                    "    case %llu: return \"%.*s\";\n",
                    llu(member->value.u), n, s);
            break;
        case vt_int:
            fprintf(out->fp,
                    "    case %lld: return \"%.*s\";\n",
                    lld(member->value.i), n, s);
            break;
        case vt_bool:
            fprintf(out->fp,
                    "    case %u: return \"%.*s\";\n",
                    member->value.b, n, s);
            break;
        default:
            break;
        }
    }
    fprintf(out->fp,
            "    default: return \"\";\n"
            "    }\n"
            "}\n");
    fprintf(out->fp, "\n");
}

static void gen_nested_root(output_t *out, fb_symbol_t *root_type, fb_symbol_t *container, fb_symbol_t *member)
{
    const char *s;
    int n;
    const char *kind;
    const char *nsc = out->nsc;
    fb_scoped_name_t snt;
    fb_scoped_name_t snc;

    fb_clear(snt);
    fb_clear(snc);
    if (!root_type) {
        return;
    }
    /*
     * Current flatc compiler only accepts tables, but here we support
     * both tables and structs in so far the parser and analyzer
     * allows for it.
     */
    switch (root_type->kind) {
    case fb_is_table:
        kind = "table_";
        break;
    case fb_is_struct:
        kind = "struct_";
        break;
    default:
        gen_panic(out, "internal error: roots can only be structs or tables");
        return;
    }
    fb_compound_name((fb_compound_type_t *)root_type, &snt);
    assert(container->kind == fb_is_table);
    fb_compound_name((fb_compound_type_t *)container, &snc);
    symbol_name(member, &n, &s);
    fprintf(out->fp, "__%snested_buffer_as_root(%s, %.*s, %s, %s)\n", nsc, snc.text, n, s, snt.text, kind);
}

static void gen_table(output_t *out, fb_compound_type_t *ct)
{
    fb_member_t *member;
    fb_symbol_t *sym;
    const char *s, *tname, *tname_ns, *tname_prefix;
    int n, r;
    int already_has_key, current_key_processed;
    const char *nsc = out->nsc;
    fb_scoped_name_t snt;
    fb_scoped_name_t snref;
    uint64_t present_id;

    assert(ct->symbol.kind == fb_is_table);

    fb_clear(snt);
    fb_clear(snref);

    fprintf(out->fp, "\n");
    fb_compound_name(ct, &snt);
    print_doc(out, "", ct->doc);
    fprintf(out->fp,
            /*
             * We don't really need the struct, but it provides better
             * type safety than a typedef void *.
             */
            "struct %s_table { uint8_t unused__; };\n"
            "\n",
            snt.text);
    fprintf(out->fp,
            /*
             * Use of file identifiers for undeclared roots is fuzzy,
             * but we need an identifer for all, so we use the one
             * defined for the current schema file and allow the user to
             * override. This avoids tedious runtime file id arguments
             * to all create calls.
             */
            "#ifndef %s_identifier\n"
            "#define %s_identifier %sidentifier\n"
            "#endif\n",
            snt.text, snt.text, nsc);
    fprintf(out->fp,
            "static inline %suoffset_t %s_vec_len(%s_vec_t vec)\n"
            "__%svec_len(vec)\n",
            nsc, snt.text, snt.text,
            nsc);
    fprintf(out->fp,
            "static inline %s_table_t %s_vec_at(%s_vec_t vec, %suoffset_t i)\n"
            "__%soffset_vec_at(%s_table_t, vec, i, 0)\n",
            snt.text, snt.text, snt.text, nsc,
            nsc, snt.text);
    fprintf(out->fp,
            "__%stable_as_root(%s)\n",
            nsc, snt.text);
    fprintf(out->fp, "\n");

    already_has_key = 0;
    for (sym = ct->members; sym; sym = sym->link) {
        current_key_processed = 0;
        member = (fb_member_t *)sym;
        present_id = member->id;
        print_doc(out, "", member->doc);
        /*
         * In flatc, there can at most one key field, and it should be
         * scalar or string. Here we export all keys using the
         * <table>_vec_find_by_<fieldname> convention and let the parser deal with
         * semantics. Keys on unsupported fields are ignored. The first
         * valid find operation is also mapped to just <table>_vec_find.
         */
        symbol_name(&member->symbol, &n, &s);
        if (member->metadata_flags & fb_f_deprecated) {
            fprintf(out->fp, "/* Skipping deprecated field: '%s_%.*s' */\n\n", snt.text, n, s);
            continue;
        }
        r = (member->metadata_flags & fb_f_required) != 0;
        switch (member->type.type) {
        case vt_scalar_type:
            tname_ns = scalar_type_ns(member->type.st, nsc);
            tname = scalar_type_name(member->type.st);
            tname_prefix = scalar_type_prefix(member->type.st);
            switch (member->value.type) {
            case vt_uint:
                fprintf(out->fp,
                    "static inline %s%s %s_%.*s(%s_table_t t)\n"
                    "__%sscalar_field(%s%s, %llu, %llu, t)\n",
                    tname_ns, tname, snt.text, n, s, snt.text,
                    nsc, nsc, tname_prefix, llu(member->id), llu(member->value.u));
                break;
            case vt_int:
                fprintf(out->fp,
                    "static inline %s%s %s_%.*s(%s_table_t t)\n"
                    "__%sscalar_field(%s%s, %llu, %lld, t)\n",
                    tname_ns, tname, snt.text, n, s, snt.text,
                    nsc, nsc, tname_prefix, llu(member->id), lld(member->value.i));
                break;
            case vt_bool:
                fprintf(out->fp,
                    "static inline %s%s %s_%.*s(%s_table_t t)\n"
                    "__%sscalar_field(%s%s, %llu, %u, t)\n",
                    tname_ns, tname, snt.text, n, s, snt.text,
                    nsc, nsc, tname_prefix, llu(member->id), member->value.b);
                break;
            case vt_float:
                fprintf(out->fp,
                    "static inline %s%s %s_%.*s(%s_table_t t)\n"
                    "__%sscalar_field(%s%s, %llu, %lf, t)\n",
                    tname_ns, tname, snt.text, n, s, snt.text,
                    nsc, nsc, tname_prefix, llu(member->id), member->value.f);
                break;
            default:
                gen_panic(out, "internal error: unexpected scalar table default value");
                continue;
            }
            if (member->metadata_flags & fb_f_key) {
                if (already_has_key) {
                    fprintf(out->fp, "/* Note: this is not the first field with a key on this table. */\n");
                }
                fprintf(out->fp,     "/* Note: find only works on vectors sorted by this field. */\n");
                fprintf(out->fp,
                        "__%sdefine_find_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
                if (out->opts->cgen_sort) {
                    fprintf(out->fp,
                        "__%sdefine_sort_by_scalar_field(%s, %.*s, %s%s, %suoffset_t)\n",
                        nsc, snt.text, n, s, tname_ns, tname, nsc);
                }
                if (!already_has_key) {
                    fprintf(out->fp,
                        "#define %s_vec_find %s_vec_find_by_%.*s\n",
                        snt.text, snt.text, n, s);
                    if (out->opts->cgen_sort) {
                        fprintf(out->fp,
                            "#define %s_vec_sort %s_vec_sort_by_%.*s\n",
                            snt.text, snt.text, n, s);
                    }
                    already_has_key = 1;
                }
                current_key_processed = 1;
            }
            break;
        case vt_vector_type:
            /* They all use a namespace. */
            tname = scalar_vector_type_name(member->type.st);
            tname_ns = nsc;
            fprintf(out->fp,
                "static inline %s%s %s_%.*s(%s_table_t t)\n"
                "__%svector_field(%s%s, %llu, t, %u)\n",
                tname_ns, tname, snt.text, n, s, snt.text,
                nsc, tname_ns, tname, llu(member->id), r);
            if (member->nest) {
                gen_nested_root(out, &member->nest->symbol, &ct->symbol, &member->symbol);
            }
            break;
        case vt_string_type:
            fprintf(out->fp,
                "static inline %sstring_t %s_%.*s(%s_table_t t)\n"
                "__%svector_field(%sstring_t, %llu, t, %u)\n",
                nsc, snt.text, n, s, snt.text,
                nsc, nsc, llu(member->id), r);
            if (member->metadata_flags & fb_f_key) {
                if (already_has_key) {
                    fprintf(out->fp, "/* Note: this is not the first field with a key on this table. */\n");
                }
                fprintf(out->fp,     "/* Note: find only works on vectors sorted by this field. */\n");
                fprintf(out->fp,
                    "static inline %suoffset_t %s_vec_find_by_%.*s(%s_vec_t vec, const char *s)\n"
                    "__%sfind_by_string_field(%s_%.*s, vec, %s_vec_at, %s_vec_len, s)\n",
                    nsc, snt.text, n, s, snt.text,
                    nsc, snt.text, n, s, snt.text, snt.text);
                fprintf(out->fp,
                    "static inline %suoffset_t %s_vec_find_n_by_%.*s(%s_vec_t vec, const char *s, int n)\n"
                    "__%sfind_by_string_n_field(%s_%.*s, vec, %s_vec_at, %s_vec_len, s, n)\n",
                    nsc, snt.text, n, s, snt.text,
                    nsc, snt.text, n, s, snt.text, snt.text);
                if (out->opts->cgen_sort) {
                    fprintf(out->fp,
                        "__%sdefine_sort_by_string_field(%s, %.*s)\n",
                        nsc, snt.text, n, s);
                }
                if (!already_has_key) {
                    fprintf(out->fp,
                        "#define %s_vec_find %s_vec_find_by_%.*s\n"
                        "#define %s_vec_find_n %s_vec_find_n_by_%.*s\n",
                        snt.text, snt.text, n, s,
                        snt.text, snt.text, n, s);
                    if (out->opts->cgen_sort) {
                        fprintf(out->fp,
                                "#define %s_vec_sort %s_vec_sort_by_%.*s\n",
                                snt.text, snt.text, n, s);
                    }
                    already_has_key = 1;
                }
                current_key_processed = 1;
            }
            break;
        case vt_vector_string_type:
            fprintf(out->fp,
                "static inline %sstring_vec_t %s_%.*s(%s_table_t t)\n"
                "__%svector_field(%sstring_vec_t, %llu, t, %u)\n",
                nsc, snt.text, n, s, snt.text,
                nsc, nsc, llu(member->id), r);
            break;
        case vt_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_struct:
                fprintf(out->fp,
                    "static inline %s_struct_t %s_%.*s(%s_table_t t)\n"
                    "__%sstruct_field(%s_struct_t, %llu, t, %u)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id), r);
                break;
            case fb_is_table:
                fprintf(out->fp,
                    "static inline %s_table_t %s_%.*s(%s_table_t t)\n"
                    "__%stable_field(%s_table_t, %llu, t, %u)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id), r);
                break;
            case fb_is_enum:
                switch (member->value.type) {
                case vt_uint:
                    fprintf(out->fp,
                        "static inline %s_enum_t %s_%.*s(%s_table_t t)\n"
                        "__%sscalar_field(%s, %llu, %llu, t)\n",
                        snref.text, snt.text, n, s, snt.text,
                        nsc, snref.text, llu(member->id), llu(member->value.u));
                    break;
                case vt_int:
                    fprintf(out->fp,
                        "static inline %s_enum_t %s_%.*s(%s_table_t t)\n"
                        "__%sscalar_field(%s, %llu, %lld, t)\n",
                        snref.text, snt.text, n, s, snt.text,
                        nsc, snref.text, llu(member->id), lld(member->value.i));
                    break;
                case vt_bool:
                    fprintf(out->fp,
                        "static inline %s_enum_t %s_%.*s(%s_table_t t)\n"
                        "__%sscalar_field(%s, %llu, %u, t)\n",
                        snref.text, snt.text, n, s, snt.text,
                        nsc, snref.text, llu(member->id), member->value.b);
                    break;
                default:
                    gen_panic(out, "internal error: unexpected enum type referenced by table");
                    continue;
                }
                if (member->metadata_flags & fb_f_key) {
                    if (already_has_key) {
                        fprintf(out->fp, "/* Note: this is not the first field with a key on this table. */\n");
                    }
                    fprintf(out->fp,     "/* Note: find only works on vectors sorted by this field. */\n");
                    fprintf(out->fp,
                            "__%sdefine_find_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
                    if (out->opts->cgen_sort) {
                        fprintf(out->fp,
                                "__%sdefine_sort_by_scalar_field(%s, %.*s, %s_enum_t, %suoffset_t)\n",
                                nsc, snt.text, n, s, snref.text, nsc);
                    }
                    if (!already_has_key) {
                        fprintf(out->fp,
                            "#define %s_vec_find %s_vec_find_by_%.*s\n",
                            snt.text, snt.text, n, s);
                        if (out->opts->cgen_sort) {
                            fprintf(out->fp,
                                    "#define %s_vec_sort %s_vec_sort_by_%.*s\n",
                                    snt.text, snt.text, n, s);
                        }
                        already_has_key = 1;
                    }
                    current_key_processed = 1;
                }
                break;
            case fb_is_union:
                present_id--;
                fprintf(out->fp,
                    "static inline %s_union_type_t %s_%.*s_type(%s_table_t t)\n"
                    "__%sscalar_field(%s, %llu, 0, t)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id) - 1);
                fprintf(out->fp,
                    "static inline %sgeneric_table_t %s_%.*s(%s_table_t t)\n"
                    "__%stable_field(%sgeneric_table_t, %llu, t, %u)\n",
                    nsc, snt.text, n, s, snt.text,
                    nsc, nsc, llu(member->id), r);
                    break;
            default:
                gen_panic(out, "internal error: unexpected compound type in table during code generation");
                break;
            }
            break;
        case vt_vector_compound_type_ref:
            fb_compound_name(member->type.ct, &snref);
            switch (member->type.ct->symbol.kind) {
            case fb_is_struct:
                fprintf(out->fp,
                    "static inline %s_vec_t %s_%.*s(%s_table_t t)\n"
                    "__%svector_field(%s_vec_t, %llu, t, %u)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id), r);
                break;
            case fb_is_table:
                fprintf(out->fp,
                    "static inline %s_vec_t %s_%.*s(%s_table_t t)\n"
                    "__%svector_field(%s_vec_t, %llu, t, %u)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id), r);
                break;
            case fb_is_enum:
                fprintf(out->fp,
                    "static inline %s_vec_t %s_%.*s(%s_table_t t)\n"
                    "__%svector_field(%s_vec_t, %llu, t, %u)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, snref.text, llu(member->id), r);
                break;
            case fb_is_union:
                gen_panic(out, "internal error: unexpected vector of union present in table");
                break;
            default:
                gen_panic(out, "internal error: unexpected vector compound type in table during code generation");
                break;
            }
            break;
        default:
            gen_panic(out, "internal error: unexpected table member type during code generation");
            break;
        }
        fprintf(out->fp,
                "static inline int %s_%.*s_is_present(%s_table_t t)\n"
                "__%sfield_present(%llu, t)\n",
                snt.text, n, s, snt.text, nsc, llu(present_id));
        if ((member->metadata_flags & fb_f_key) && !current_key_processed) {
            fprintf(out->fp,
                "/* Note: field has key, but there is no support for find by fields of this type. */\n");
            /*
             * If the first key already exists, but was for an unsupported
             * type, we do not map the next possible key to generic find.
             */
            already_has_key = 1;
        }
        fprintf(out->fp, "\n");
    }
}

int fb_gen_c_reader(output_t *out)
{
    fb_symbol_t *sym;
    fb_compound_type_t *ct;

    gen_pretext(out);

    for (ct = out->S->ordered_structs; ct; ct = ct->order) {
        gen_forward_decl(out, ct);
    }
    fprintf(out->fp, "\n");
    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_table:
            gen_forward_decl(out, (fb_compound_type_t *)sym);
            break;
        }
    }
    fprintf(out->fp, "\n");
    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
         /* Enums must come before structs in case they are referenced. */
        case fb_is_enum:
            gen_enum(out, (fb_compound_type_t *)sym);
            break;
        }
    }
    fprintf(out->fp, "\n");
    /* Generate structs in topologically sorted order. */
    for (ct = out->S->ordered_structs; ct; ct = ct->order) {
            gen_struct(out, ct);
    }
    for (sym = out->S->symbols; sym; sym = sym->link) {
        switch (sym->kind) {
        case fb_is_enum:
        case fb_is_struct:
            /* Already generated. */
            break;
        case fb_is_union:
            gen_enum(out, (fb_compound_type_t *)sym);
            break;
        case fb_is_table:
            gen_table(out, (fb_compound_type_t *)sym);
            break;
        default:
            gen_panic(out, "internal error: unexpected schema component");
            break;
        }
    }
    fprintf(out->fp, "\n");
    gen_footer(out);
    return 0;
}

static int open_output_file(output_t *out, const char *name, int len, const char *ext)
{
    char *path;
    int ret;
    const char *prefix = out->opts->outpath ? out->opts->outpath : "";
    int prefix_len = strlen(prefix);

    if (out->opts->gen_stdout) {
        out->fp = stdout;
        return 0;
    }
    checkmem((path = fb_create_join_path(prefix, prefix_len, name, len, ext, 1)));
    out->fp = fopen(path, "wb");
    ret = 0;
    if (!out->fp) {
        fprintf(stderr, "error opening file for write: %s\n", path);
        ret = -1;
    }
    free(path);
    return ret;
}

static void close_output_file(output_t *out)
{
    if (out->fp != stdout && out->fp) {
        fclose(out->fp);
    }
    out->fp = 0;
}

int fb_copy_scope(fb_scope_t *scope, char *buf)
{
    int n, len;
    fb_ref_t *name;

    len = scope->prefix.len;
    for (name = scope->name; name; name = name->link) {
        n = name->ident->len;
        len += n + 1;
    }
    if (len > FLATCC_NAMESPACE_MAX + 1) {
        buf[0] = '\0';
        return -1;
    }
    len = scope->prefix.len;
    memcpy(buf, scope->prefix.s, len);
    for (name = scope->name; name; name = name->link) {
        n = name->ident->len;
        memcpy(buf + len, name->ident->text, n);
        len += n + 1;
        buf[len - 1] = '_';
    }
    buf[len + 1] = '\0';
    return len;
}

static int init_output(output_t *out, fb_options_t *opts)
{
    const char *nsc;
    char *p;
    int n;

    memset(out, 0, sizeof(*out));
    out->opts = opts;
    nsc = opts->nsc;
    if (nsc) {
        n = strlen(opts->nsc);
        if (n > FLATCC_NAMESPACE_MAX) {
            fprintf(stderr, "common namespace argument is limited to %i characters\n", (int)FLATCC_NAMESPACE_MAX);
            return -1;
        }
    } else {
        nsc = FLATCC_DEFAULT_NAMESPACE_COMMON;
        n = strlen(nsc);
    }
    strncpy(out->nsc, nsc, FLATCC_NAMESPACE_MAX);
    out->nsc[FLATCC_NAMESPACE_MAX] = '\0';
    if (n) {
        out->nsc[n] = '_';
        out->nsc[n + 1] = '\0';
    }
    strcpy(out->nscup, out->nsc);
    for (p = out->nscup; *p; ++p) {
        *p = toupper(*p);
    }
    if (p != out->nscup) {
      p[-1] = '\0'; /* No trailing _ */
    }
    return 0;
}

int fb_codegen_common_c(fb_options_t *opts)
{
    output_t output, *out;
    out = &output;
    int ret, nsc_len;

    if (init_output(out, opts)) {
        return -1;
    }
    nsc_len = strlen(output.nsc) - 1;
    ret = 0;
    if (opts->cgen_common_reader) {
        if (open_output_file(out, out->nsc, nsc_len, "_common_reader.h")) {
            return -1;
        }
        ret = fb_gen_common_c_header(out);
        close_output_file(out);
    }
    if (!ret && opts->cgen_common_builder) {
        if (open_output_file(out, out->nsc, nsc_len, "_common_builder.h")) {
            return -1;
        }
        fb_gen_common_c_builder_header(out);
        close_output_file(out);
    }
    return ret;
}

int fb_codegen_c(fb_options_t *opts, fb_schema_t *S)
{
    output_t output, *out;
    out = &output;
    int ret, basename_len;

    if (init_output(out, opts)) {
        return -1;
    }
    out->S = S;
    out->current_scope = fb_scope_table_find(&S->root_schema->scope_index, 0, 0);
    basename_len = strlen(out->S->basename);
    ret = 0;
    if (opts->cgen_reader) {
        if (open_output_file(out, out->S->basename, basename_len, "_reader.h")) {
            return -1;
        }
        ret = fb_gen_c_reader(out);
        close_output_file(out);
    }
    if (!ret && opts->cgen_builder) {
        if (open_output_file(out, out->S->basename, basename_len, "_builder.h")) {
            return -1;
        }
        ret = fb_gen_c_builder(out);
        close_output_file(out);
    }
    if (!ret && opts->cgen_verifier) {
        if (open_output_file(out, out->S->basename, basename_len, "_verifier.h")) {
            return -1;
        }
        ret = fb_gen_c_verifier(out);
        close_output_file(out);
    }
    return ret;
}
