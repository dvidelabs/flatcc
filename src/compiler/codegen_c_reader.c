#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "codegen_c.h"
#include "codegen_c_sort.h"

#define llu(x) (long long unsigned int)(x)
#define lld(x) (long long int)(x)


/*
 * Use of file identifiers for undeclared roots is fuzzy, but we need an
 * identifer for all, so we use the one defined for the current schema
 * file and allow the user to override. This avoids tedious runtime file
 * id arguments to all create calls.
 *
 * As later addition to FlatBuffers, type hashes may replace file
 * identifiers when explicitly stated. These are FNV-1a hashes of the
 * fully qualified type name (dot separated).
 *
 * We generate the type hash both as a native integer constants for use
 * in switch statements, and encoded as a little endian C string for use
 * as a file identifier.
 */
static void print_type_identifier(fb_output_t *out, const char *name, uint32_t type_hash)
{
    uint8_t buf[17];
    uint8_t *p;
    uint8_t x;
    int i;
    const char *nsc = out->nsc;

    fprintf(out->fp,
            "#ifndef %s_identifier\n"
            "#define %s_identifier %sidentifier\n"
            "#endif\n",
            name, name, nsc);
    fprintf(out->fp,
        "#define %s_type_hash ((%sthash_t)0x%lx)\n",
        name, nsc, (unsigned long)(type_hash));
    p = buf;
    i = 4;
    while (i--) {
        *p++ = '\\';
        *p++ = 'x';
        x = type_hash & 0x0f;
        x += x > 9 ? 'a' - 10 : '0';
        type_hash >>= 4;
        p[1] = x;
        x = type_hash & 0x0f;
        x += x > 9 ? 'a' - 10 : '0';
        type_hash >>= 4;
        p[0] = x;
        p += 2;
    }
    *p = '\0';
    fprintf(out->fp,
        "#define %s_type_identifier \"%s\"\n",
        name, buf);
}

/* Finds first occurrence of matching key when vector is sorted on the named field. */
static void gen_find(fb_output_t *out)
{
    const char *nsc = out->nsc;

    /*
     * E: Element accessor (elem = E(vector, index)).
     * L: Length accessor (length = L(vector)).
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
        "#include <string.h>\n"
        "static size_t %snot_found = (size_t)-1;\n"
        "static size_t %send = (size_t)-1;\n"
        "#define __%sidentity(n) (n)\n"
        "#define __%smin(a, b) ((a) < (b) ? (a) : (b))\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/* Subtraction doesn't work for unsigned types. */\n"
        "#define __%sscalar_cmp(x, y, n) ((x) < (y) ? -1 : (x) > (y))\n"
        "static inline int __%sstring_n_cmp(%sstring_t v, const char *s, size_t n)\n"
        "{ size_t nv = %sstring_len(v); int x = strncmp(v, s, nv < n ? nv : n);\n"
        "  return x != 0 ? x : nv < n ? -1 : nv > n; }\n"
        "/* `n` arg unused, but needed by string find macro expansion. */\n"
        "static inline int __%sstring_cmp(%sstring_t v, const char *s, size_t n) { (void)n; return strcmp(v, s); }\n",
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "/* A = identity if searching scalar vectors rather than key fields. */\n"
        "/* Returns lowest matching index or not_found. */\n"
        "#define __%sfind_by_field(A, V, E, L, K, Kn, T, D)\\\n"
        "{ T v; size_t a = 0, b, m; if (!(b = L(V))) { return %snot_found; }\\\n"
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
        nsc, nsc, nsc);
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
        "static inline size_t N ## _vec_find_by_ ## NK(N ## _vec_t vec, TK key)\\\n"
        "__%sfind_by_scalar_field(N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, TK)\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scalar_find(N, T)\\\n"
        "static inline size_t N ## _vec_find(N ## _vec_t vec, T key)\\\n"
        "__%sfind_by_scalar_field(__%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_find_by_string_field(N, NK) \\\n"
        "/* Note: find only works on vectors sorted by this field. */\\\n"
        "static inline size_t N ## _vec_find_by_ ## NK(N ## _vec_t vec, const char *s)\\\n"
        "__%sfind_by_string_field(N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s)\\\n"
        "static inline size_t N ## _vec_find_n_by_ ## NK(N ## _vec_t vec, const char *s, int n)\\\n"
        "__%sfind_by_string_n_field(N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s, n)\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_default_find_by_scalar_field(N, NK, TK)\\\n"
        "static inline size_t N ## _vec_find(N ## _vec_t vec, TK key)\\\n"
        "{ return N ## _vec_find_by_ ## NK(vec, key); }\n",
        nsc);
    fprintf(out->fp,
        "#define __%sdefine_default_find_by_string_field(N, NK) \\\n"
        "static inline size_t N ## _vec_find(N ## _vec_t vec, const char *s)\\\n"
        "{ return N ## _vec_find_by_ ## NK(vec, s); }\\\n"
        "static inline size_t N ## _vec_find_n(N ## _vec_t vec, const char *s, int n)\\\n"
        "{ return N ## _vec_find_n_by_ ## NK(vec, s, n); }\n",
        nsc);
}

/* Linearly finds first occurrence of matching key, doesn't require vector to be sorted. */
static void gen_scan(fb_output_t *out)
{
    const char *nsc = out->nsc;

    /*
     * E: Element accessor (elem = E(vector, index)).
     * L: Length accessor (length = L(vector)).
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
        "/* A = identity if searching scalar vectors rather than key fields. */\n"
        "/* Returns lowest matching index or not_found. */\n"
        "#define __%sscan_by_field(b, e, A, V, E, L, K, Kn, T, D)\\\n"
        "{ T v; size_t i;\\\n"
        "  for (i = b; i < e; ++i) {\\\n"
        "    v = A(E(V, i));\\\n"
        "    if (D(v, (K), (Kn)) == 0) {\\\n"
        "       return i;\\\n"
        "    }\\\n"
        "  }\\\n"
        "  return %snot_found;\\\n"
        "}\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%srscan_by_field(b, e, A, V, E, L, K, Kn, T, D)\\\n"
        "{ T v; size_t i = e;\\\n"
        "  while (i-- > b) {\\\n"
        "    v = A(E(V, i));\\\n"
        "    if (D(v, (K), (Kn)) == 0) {\\\n"
        "       return i;\\\n"
        "    }\\\n"
        "  }\\\n"
        "  return %snot_found;\\\n"
        "}\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%sscan_by_scalar_field(b, e, A, V, E, L, K, T)\\\n"
        "__%sscan_by_field(b, e, A, V, E, L, K, 0, T, __%sscalar_cmp)\n"
        "#define __%sscan_by_string_field(b, e, A, V, E, L, K)\\\n"
        "__%sscan_by_field(b, e, A, V, E, L, K, 0, %sstring_t, __%sstring_cmp)\n"
        "#define __%sscan_by_string_n_field(b, e, A, V, E, L, K, Kn)\\\n"
        "__%sscan_by_field(b, e, A, V, E, L, K, Kn, %sstring_t, __%sstring_n_cmp)\n",
        nsc, nsc, nsc, nsc, nsc,
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%srscan_by_scalar_field(b, e, A, V, E, L, K, T)\\\n"
        "__%srscan_by_field(b, e, A, V, E, L, K, 0, T, __%sscalar_cmp)\n"
        "#define __%srscan_by_string_field(b, e, A, V, E, L, K)\\\n"
        "__%srscan_by_field(b, e, A, V, E, L, K, 0, %sstring_t, __%sstring_cmp)\n"
        "#define __%srscan_by_string_n_field(b, e, A, V, E, L, K, Kn)\\\n"
        "__%srscan_by_field(b, e, A, V, E, L, K, Kn, %sstring_t, __%sstring_n_cmp)\n",
        nsc, nsc, nsc, nsc, nsc,
        nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scan_by_scalar_field(N, NK, T)\\\n"
        "static inline size_t N ## _vec_scan_by_ ## NK(N ## _vec_t vec, T key)\\\n"
        "__%sscan_by_scalar_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_scan_ex_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, T key)\\\n"
        "__%sscan_by_scalar_field(begin, __%smin(end, N ## _vec_len(vec)), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_rscan_by_ ## NK(N ## _vec_t vec, T key)\\\n"
        "__%srscan_by_scalar_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_rscan_ex_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, T key)\\\n"
        "__%srscan_by_scalar_field(begin, __%smin(end, N ## _vec_len(vec)), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, key, T)\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scalar_scan(N, T)\\\n"
        "static inline size_t N ## _vec_scan(N ## _vec_t vec, T key)\\\n"
        "__%sscan_by_scalar_field(0, N ## _vec_len(vec), __%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_scan_ex(N ## _vec_t vec, size_t begin, size_t end, T key)\\\n"
        "__%sscan_by_scalar_field(begin, __%smin(end, N ## _vec_len(vec)), __%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_rscan(N ## _vec_t vec, T key)\\\n"
        "__%srscan_by_scalar_field(0, N ## _vec_len(vec), __%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\\\n"
        "static inline size_t N ## _vec_rscan_ex(N ## _vec_t vec, size_t begin, size_t end, T key)\\\n"
        "__%srscan_by_scalar_field(begin, __%smin(end, N ## _vec_len(vec)), __%sidentity, vec, N ## _vec_at, N ## _vec_len, key, T)\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scan_by_string_field(N, NK) \\\n"
        "static inline size_t N ## _vec_scan_by_ ## NK(N ## _vec_t vec, const char *s)\\\n"
        "__%sscan_by_string_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s)\\\n"
        "static inline size_t N ## _vec_scan_n_by_ ## NK(N ## _vec_t vec, const char *s, int n)\\\n"
        "__%sscan_by_string_n_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s, n)\\\n"
        "static inline size_t N ## _vec_scan_ex_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, const char *s)\\\n"
        "__%sscan_by_string_field(begin, __%smin(end, N ## _vec_len(vec)), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s)\\\n"
        "static inline size_t N ## _vec_scan_ex_n_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, const char *s, int n)\\\n"
        "__%sscan_by_string_n_field(begin, __%smin( end, N ## _vec_len(vec) ), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s, n)\\\n"
        "static inline size_t N ## _vec_rscan_by_ ## NK(N ## _vec_t vec, const char *s)\\\n"
        "__%srscan_by_string_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s)\\\n"
        "static inline size_t N ## _vec_rscan_n_by_ ## NK(N ## _vec_t vec, const char *s, int n)\\\n"
        "__%srscan_by_string_n_field(0, N ## _vec_len(vec), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s, n)\\\n"
        "static inline size_t N ## _vec_rscan_ex_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, const char *s)\\\n"
        "__%srscan_by_string_field(begin, __%smin(end, N ## _vec_len(vec)), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s)\\\n"
        "static inline size_t N ## _vec_rscan_ex_n_by_ ## NK(N ## _vec_t vec, size_t begin, size_t end, const char *s, int n)\\\n"
        "__%srscan_by_string_n_field(begin, __%smin( end, N ## _vec_len(vec) ), N ## _ ## NK, vec, N ## _vec_at, N ## _vec_len, s, n)\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_default_scan_by_scalar_field(N, NK, TK)\\\n"
        "static inline size_t N ## _vec_scan(N ## _vec_t vec, TK key)\\\n"
        "{ return N ## _vec_scan_by_ ## NK(vec, key); }\\\n"
        "static inline size_t N ## _vec_scan_ex(N ## _vec_t vec, size_t begin, size_t end, TK key)\\\n"
        "{ return N ## _vec_scan_ex_by_ ## NK(vec, begin, end, key); }\\\n"
        "static inline size_t N ## _vec_rscan(N ## _vec_t vec, TK key)\\\n"
        "{ return N ## _vec_rscan_by_ ## NK(vec, key); }\\\n"
        "static inline size_t N ## _vec_rscan_ex(N ## _vec_t vec, size_t begin, size_t end, TK key)\\\n"
        "{ return N ## _vec_rscan_ex_by_ ## NK(vec, begin, end, key); }\n",
        nsc);
    fprintf(out->fp,
        "#define __%sdefine_default_scan_by_string_field(N, NK) \\\n"
        "static inline size_t N ## _vec_scan(N ## _vec_t vec, const char *s)\\\n"
        "{ return N ## _vec_scan_by_ ## NK(vec, s); }\\\n"
        "static inline size_t N ## _vec_scan_n(N ## _vec_t vec, const char *s, int n)\\\n"
        "{ return N ## _vec_scan_n_by_ ## NK(vec, s, n); }\\\n"
        "static inline size_t N ## _vec_scan_ex(N ## _vec_t vec, size_t begin, size_t end, const char *s)\\\n"
        "{ return N ## _vec_scan_ex_by_ ## NK(vec, begin, end, s); }\\\n"
        "static inline size_t N ## _vec_scan_ex_n(N ## _vec_t vec, size_t begin, size_t end, const char *s, int n)\\\n"
        "{ return N ## _vec_scan_ex_n_by_ ## NK(vec, begin, end, s, n); }\\\n"
        "static inline size_t N ## _vec_rscan(N ## _vec_t vec, const char *s)\\\n"
        "{ return N ## _vec_rscan_by_ ## NK(vec, s); }\\\n"
        "static inline size_t N ## _vec_rscan_n(N ## _vec_t vec, const char *s, int n)\\\n"
        "{ return N ## _vec_rscan_n_by_ ## NK(vec, s, n); }\\\n"
        "static inline size_t N ## _vec_rscan_ex(N ## _vec_t vec, size_t begin, size_t end, const char *s)\\\n"
        "{ return N ## _vec_rscan_ex_by_ ## NK(vec, begin, end, s); }\\\n"
        "static inline size_t N ## _vec_rscan_ex_n(N ## _vec_t vec, size_t begin, size_t end, const char *s, int n)\\\n"
        "{ return N ## _vec_rscan_ex_n_by_ ## NK(vec, begin, end, s, n); }\n",
        nsc);
}

static void gen_helpers(fb_output_t *out)
{
    const char *nsc = out->nsc;

    fprintf(out->fp,
        /*
         * Include the basic primitives for accessing flatbuffer datatypes independent
         * of endianness.
         *
         * The included file must define the basic types and accessors
         * prefixed with the common namespace which by default is
         * "flatbuffers_".
         */
        "#include \"flatcc/flatcc_flatbuffers.h\"\n"
        "\n\n");
    /*
     * The remapping of basic types to the common namespace makes it
     * possible to have different definitions. The generic
     * `flatbuffers_uoffset_t` etc. cannot be trusted to have one specific
     * size since it depends on the included `flatcc/flatcc_types.h`
     * filer, but the namespace prefixed types can be trusted if used carefully.
     * For example the common namespace could be `flatbuffers_large_`
     * when allowing for 64 bit offsets.
     */
    if (strcmp(nsc, "flatbuffers_")) {
        fprintf(out->fp,
                "typedef flatbuffers_uoffset_t %suoffset_t;\n"
                "typedef flatbuffers_soffset_t %ssoffset_t;\n"
                "typedef flatbuffers_voffset_t %svoffset_t;\n"
                "typedef flatbuffers_utype_t %sutype_t;\n"
                "typedef flatbuffers_bool_t %sbool_t;\n"
                "\n",
                nsc, nsc, nsc, nsc, nsc);
        fprintf(out->fp,
                "#define %sendian flatbuffers_endian\n"
                "__flatcc_define_basic_scalar_accessors(%s, flatbuffers_endian)"
                "__flatcc_define_integer_accessors(%sbool, flatbuffers_bool_t,\\\n"
                "        FLATBUFFERS_BOOL_WIDTH, flatbuffers_endian)\n",
                nsc, nsc, nsc);
        fprintf(out->fp,
                "__flatcc_define_integer_accessors(__%suoffset, flatbuffers_uoffset_t,\n"
                "        FLATBUFFERS_UOFFSET_WIDTH, flatbuffers_endian)\n"
                "__flatcc_define_integer_accessors(__%ssoffset, flatbuffers_soffset_t,\n"
                "        FLATBUFFERS_SOFFSET_WIDTH, flatbuffers_endian)\n"
                "__flatcc_define_integer_accessors(__%svoffset, flatbuffers_voffset_t,\n"
                "        FLATBUFFERS_VOFFSET_WIDTH, flatbuffers_endian)\n"
                "__flatcc_define_integer_accessors(__%sutype, flatbuffers_utype_t,\n"
                "        FLATBUFFERS_UTYPE_WIDTH, flatbuffers_endian)\n"
                "__flatcc_define_integer_accessors(__%sthash, flatbuffers_thash_t,\n"
                "        FLATBUFFERS_THASH_WIDTH, flatbuffers_endian)\n",
                nsc, nsc, nsc, nsc, nsc);
        fprintf(out->fp,
                "#ifndef %s_WRAP_NAMESPACE\n"
                "#define %s_WRAP_NAMESPACE(ns, x) ns ## _ ## x\n"
                "#endif\n",
                out->nscup, out->nscup);
    }
    /* Build out a more elaborate type system based in the primitives included. */
    fprintf(out->fp,
        "#define __%sread_scalar_at_byteoffset(N, p, o) N ## _read_from_pe((uint8_t *)(p) + (o))\n"
        "#define __%sread_scalar(N, p) N ## _read_from_pe(p)\n",
        nsc, nsc);
    fprintf(out->fp,
        "#define __%sread_vt(ID, offset, t)\\\n"
        "%svoffset_t offset = 0;\\\n"
        "{   %svoffset_t id, *vt;\\\n"
        "    assert(t != 0 && \"null pointer table access\");\\\n"
        "    id = ID;\\\n"
        "    vt = (%svoffset_t *)((uint8_t *)(t) -\\\n"
        "        __%ssoffset_read_from_pe(t));\\\n"
        "    if (__%svoffset_read_from_pe(vt) >= sizeof(vt[0]) * (id + 3)) {\\\n"
        "        offset = __%svoffset_read_from_pe(vt + id + 2);\\\n"
        "    }\\\n"
        "}\n",
        nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sfield_present(ID, t) { __%sread_vt(ID, offset, t) return offset != 0; }\n",
            nsc, nsc);
    fprintf(out->fp,
        "#define __%sunion_type_field(N, ID, V, t)\\\n"
        "{\\\n"
        "    __%sread_vt(ID, offset, t)\\\n"
        "    return offset ? __%sread_scalar_at_byteoffset(N, t, offset) : V;\\\n"
        "}\n",
        nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%sdefine_scalar_field(ID, N, NK, TK, T, V)\\\n"
        "static inline T N ## _ ## NK (N ## _table_t t)\\\n"
        "{ __%sread_vt(ID, offset, t)\\\n"
        "  return offset ? __%sread_scalar_at_byteoffset(TK, t, offset) : V;\\\n"
        "}\\\n"
        "static inline int N ## _ ## NK ## _is_present(N ## _table_t t)\\\n"
        "{ __%sread_vt(ID, offset, t) return offset != 0; }",
        nsc, nsc, nsc, nsc);
        if (out->opts->allow_scan_for_all_fields) {
            fprintf(out->fp, "\\\n__%sdefine_scan_by_scalar_field(N, NK, T)\n", nsc);
        } else {
            fprintf(out->fp, "\n");
        }
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
        "              __%suoffset_read_from_pe(elem));\\\n"
        "    }\\\n"
        "    assert(!(r) && \"required field missing\");\\\n"
        "    return 0;\\\n"
        "}\n",
        nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%svector_field(T, ID, t, r) __%soffset_field(T, ID, t, r, sizeof(%suoffset_t))\n"
        "#define __%stable_field(T, ID, t, r) __%soffset_field(T, ID, t, r, 0)\n",
        nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "#define __%svec_len(vec)\\\n"
        "{ return (vec) ? (size_t)__%suoffset_read_from_pe((flatbuffers_uoffset_t *)vec - 1) : 0; }\n"
        "#define __%sstring_len(s) __%svec_len(s)\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
        "static inline size_t %svec_len(const void *vec)\n"
        "__%svec_len(vec)\n",
        nsc, nsc);
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
        "{ const %suoffset_t *elem = (vec) + (i);\\\n"
        "  assert(%svec_len(vec) > (i) && \"index out of range\");\\\n"
        "  return (T)((uint8_t *)(elem) + (size_t)__%suoffset_read_from_pe(elem) + adjust); }\n",
        nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sdefine_scalar_vec_len(N) \\\n"
            "static inline size_t N ## _vec_len(N ##_vec_t vec)\\\n"
            "{ return %svec_len(vec); }\n",
            nsc, nsc);
    fprintf(out->fp,
            "#define __%sdefine_scalar_vec_at(N, T) \\\n"
            "static inline T N ## _vec_at(N ## _vec_t vec, size_t i)\\\n"
            "__%sscalar_vec_at(N, vec, i)\n",
            nsc, nsc);
    fprintf(out->fp,
            "typedef const char *%sstring_t;\n"
            "static inline size_t %sstring_len(%sstring_t s)\n"
            "__%sstring_len(s)\n",
            nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "typedef const %suoffset_t *%sstring_vec_t;\n"
            "typedef %suoffset_t *%sstring_mutable_vec_t;\n"
            "static inline size_t %sstring_vec_len(%sstring_vec_t vec)\n"
            "__%svec_len(vec)\n"
            "static inline %sstring_t %sstring_vec_at(%sstring_vec_t vec, size_t i)\n"
            "__%soffset_vec_at(%sstring_t, vec, i, sizeof(vec[0]))\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "typedef const void *%sgeneric_table_t;\n",
            nsc);
    gen_find(out);
    gen_scan(out);
    if (out->opts->cgen_sort) {
        gen_sort(out);
    } else {
        fprintf(out->fp, "/* sort disabled */\n");
    }
    fprintf(out->fp,
            "#define __%sdefine_scalar_vector(N, T)\\\n"
            "typedef const T *N ## _vec_t;\\\n"
            "typedef T *N ## _mutable_vec_t;\\\n"
            "__%sdefine_scalar_vec_len(N)\\\n"
            "__%sdefine_scalar_vec_at(N, T)\\\n"
            "__%sdefine_scalar_find(N, T)\\\n"
            "__%sdefine_scalar_scan(N, T)\\\n",
            nsc, nsc, nsc, nsc, nsc);
    if (out->opts->cgen_sort) {
        fprintf(out->fp, "\\\n__%sdefine_scalar_sort(N, T)\n", nsc);
    }
    fprintf(out->fp, "\n");
    /* Elaborate on the included basic type system. */
    fprintf(out->fp,
            "#define __%sdefine_integer_type(N, T, W)\\\n"
            "__flatcc_define_integer_accessors(N, T, W, %sendian)\\\n"
            "__%sdefine_scalar_vector(N, T)\n",
            nsc, nsc, nsc);
    fprintf(out->fp,
            "__%sdefine_scalar_vector(%sbool, %sbool_t)\n"
            "__%sdefine_scalar_vector(%suint8, uint8_t)\n"
            "__%sdefine_scalar_vector(%sint8, int8_t)\n"
            "__%sdefine_scalar_vector(%suint16, uint16_t)\n"
            "__%sdefine_scalar_vector(%sint16, int16_t)\n"
            "__%sdefine_scalar_vector(%suint32, uint32_t)\n"
            "__%sdefine_scalar_vector(%sint32, int32_t)\n"
            "__%sdefine_scalar_vector(%suint64, uint64_t)\n"
            "__%sdefine_scalar_vector(%sint64, int64_t)\n"
            "__%sdefine_scalar_vector(%sfloat, float)\n"
            "__%sdefine_scalar_vector(%sdouble, double)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "static inline size_t %sstring_vec_find(%sstring_vec_t vec, const char *s)\n"
            "__%sfind_by_string_field(__%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline size_t %sstring_vec_find_n(%sstring_vec_t vec, const char *s, size_t n)\n"
            "__%sfind_by_string_n_field(__%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "static inline size_t %sstring_vec_scan(%sstring_vec_t vec, const char *s)\n"
            "__%sscan_by_string_field(0, %sstring_vec_len(vec), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline size_t %sstring_vec_scan_n(%sstring_vec_t vec, const char *s, size_t n)\n"
            "__%sscan_by_string_n_field(0, %sstring_vec_len(vec), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n"
            "static inline size_t %sstring_vec_scan_ex(%sstring_vec_t vec, size_t begin, size_t end, const char *s)\n"
            "__%sscan_by_string_field(begin, __%smin(end, %sstring_vec_len(vec)), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline size_t %sstring_vec_scan_ex_n(%sstring_vec_t vec, size_t begin, size_t end, const char *s, size_t n)\n"
            "__%sscan_by_string_n_field(begin, __%smin(end, %sstring_vec_len(vec)), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n"
            "static inline size_t %sstring_vec_rscan(%sstring_vec_t vec, const char *s)\n"
            "__%srscan_by_string_field(0, %sstring_vec_len(vec), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline size_t %sstring_vec_rscan_n(%sstring_vec_t vec, const char *s, size_t n)\n"
            "__%srscan_by_string_n_field(0, %sstring_vec_len(vec), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n"
            "static inline size_t %sstring_vec_rscan_ex(%sstring_vec_t vec, size_t begin, size_t end, const char *s)\n"
            "__%srscan_by_string_field(begin, __%smin(end, %sstring_vec_len(vec)), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s)\n"
            "static inline size_t %sstring_vec_rscan_ex_n(%sstring_vec_t vec, size_t begin, size_t end, const char *s, size_t n)\n"
            "__%srscan_by_string_n_field(begin, __%smin(end, %sstring_vec_len(vec)), __%sidentity, vec, %sstring_vec_at, %sstring_vec_len, s, n)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc,
            nsc, nsc, nsc, nsc);
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
            "{ %sthash_t id, id2 = 0; if (fid == 0) { return 1; };\n"
            "  strncpy((char *)&id2, fid, sizeof(id2));\n"
            "  /* Identifier strings are always considered little endian. */\n"
            "  id2 = __%sthash_cast_from_le(id2);\n"
            "  id = __%sthash_read_from_pe(((%suoffset_t *)buffer) + 1);\n"
            "  return id2 == 0 || id == id2; }\n"
            "static inline int %shas_type_hash(const void *buffer, %sthash_t thash)\n"
            "{ return thash == 0 || (__%sthash_read_from_pe((%suoffset_t *)buffer + 1) == thash); }\n\n"
            "static inline %sthash_t %sget_type_hash(const void *buffer)\n"
            "{ return __%sthash_read_from_pe((flatbuffers_uoffset_t *)buffer + 1); }\n\n"
            "#define %sverify_endian() %shas_identifier(\"\\x00\\x00\\x00\\x00\" \"1234\", \"1234\")\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "static inline void *%sread_size_prefix(void *b, size_t *size_out)\n"
            "{ if (size_out) { *size_out = (size_t)__%suoffset_read_from_pe(b); }\n"
            "  return (uint8_t *)b + sizeof(%suoffset_t); }\n", nsc, nsc, nsc);
    fprintf(out->fp,
            "/* Null file identifier accepts anything, otherwise fid should be 4 characters. */\n"
            "#define __%sread_root(T, K, buffer, fid)\\\n"
            "  ((!buffer || !%shas_identifier(buffer, fid)) ? 0 :\\\n"
            "  ((T ## _ ## K ## t)(((uint8_t *)buffer) +\\\n"
            "    __%suoffset_read_from_pe(buffer))))\n"
            "#define __%sread_typed_root(T, K, buffer, thash)\\\n"
            "  ((!buffer || !%shas_type_hash(buffer, thash)) ? 0 :\\\n"
            "  ((T ## _ ## K ## t)(((uint8_t *)buffer) +\\\n"
            "    __%suoffset_read_from_pe(buffer))))\n",
            nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%snested_buffer_as_root(C, N, T, K)\\\n"
            "static inline T ## _ ## K ## t C ## _ ## N ## _as_root_with_identifier(C ## _ ## table_t t, const char *fid)\\\n"
            "{ const uint8_t *buffer = C ## _ ## N(t); return __%sread_root(T, K, buffer, fid); }\\\n"
            "static inline T ## _ ## K ## t C ## _ ## N ## _as_typed_root(C ## _ ## table_t t)\\\n"
            "{ const uint8_t *buffer = C ## _ ## N(t); return __%sread_root(T, K, buffer, C ## _ ## type_identifier); }\\\n"
            "static inline T ## _ ## K ## t C ## _ ## N ## _as_root(C ## _ ## table_t t)\\\n"
            "{ const char *fid = T ## _identifier;\\\n"
            "  const uint8_t *buffer = C ## _ ## N(t); return __%sread_root(T, K, buffer, fid); }\n",
            nsc, nsc, nsc, nsc);
    fprintf(out->fp,
            "#define __%sbuffer_as_root(N, K)\\\n"
            "static inline N ## _ ## K ## t N ## _as_root_with_identifier(const void *buffer, const char *fid)\\\n"
            "{ return __%sread_root(N, K, buffer, fid); }\\\n"
            "static inline N ## _ ## K ## t N ## _as_root_with_type_hash(const void *buffer, %sthash_t thash)\\\n"
            "{ return __%sread_typed_root(N, K, buffer, thash); }\\\n"
            "static inline N ## _ ## K ## t N ## _as_root(const void *buffer)\\\n"
            "{ const char *fid = N ## _identifier;\\\n"
            "  return __%sread_root(N, K, buffer, fid); }\\\n"
            "static inline N ## _ ## K ## t N ## _as_typed_root(const void *buffer)\\\n"
            "{ return __%sread_typed_root(N, K, buffer, N ## _type_hash); }\n"
            "#define __%sstruct_as_root(N) __%sbuffer_as_root(N, struct_)\n"
            "#define __%stable_as_root(N) __%sbuffer_as_root(N, table_)\n",
            nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc, nsc);
    fprintf(out->fp, "\n");
}

int fb_gen_common_c_header(fb_output_t *out)
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
    gen_helpers(out);
    gen_pragma_pop(out);
    fprintf(out->fp,
        "#endif /* %s_COMMON_H */\n",
        nscup);
    return 0;
}

static void gen_pretext(fb_output_t *out)
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
    fprintf(out->fp, "\n");
}

static void gen_footer(fb_output_t *out)
{
    gen_pragma_pop(out);
    fprintf(out->fp, "#endif /* %s_READER_H */\n", out->S->basenameup);
}

static void gen_forward_decl(fb_output_t *out, fb_compound_type_t *ct)
{
    fb_scoped_name_t snt;
    const char *nsc = out->nsc;

    fb_clear(snt);

    assert(ct->symbol.kind == fb_is_struct || ct->symbol.kind == fb_is_table);

    fb_compound_name(ct, &snt);
    if (ct->symbol.kind == fb_is_struct) {
        if (ct->size == 0) {
            fprintf(out->fp, "typedef void %s_t; /* empty struct */\n",
                    snt.text);
        } else {
            fprintf(out->fp, "typedef struct %s %s_t;\n",
                    snt.text, snt.text);
        }
        fprintf(out->fp, "typedef const %s_t *%s_struct_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef %s_t *%s_mutable_struct_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef const %s_t *%s_vec_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef %s_t *%s_mutable_vec_t;\n",
                snt.text, snt.text);
    } else {
        fprintf(out->fp, "typedef const struct %s_table *%s_table_t;\n",
                snt.text, snt.text);
        fprintf(out->fp, "typedef const %suoffset_t *%s_vec_t;\n", nsc, snt.text);
        fprintf(out->fp, "typedef %suoffset_t *%s_mutable_vec_t;\n", nsc, snt.text);
    }
}

static inline void print_doc(fb_output_t *out, const char *indent, fb_doc_t *doc)
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
                fprintf(out->fp, "%s/** ", indent);
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

static void gen_struct(fb_output_t *out, fb_compound_type_t *ct)
{
    fb_member_t *member;
    fb_symbol_t *sym;
    unsigned align;
    size_t offset = 0;
    const char *tname, *tname_ns, *tname_prefix;
    int n;
    const char *s;
    unsigned pad_index = 0, deprecated_index = 0, pad;
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
    if (ct->size == 0) {
        /*
         * This implies that sizeof(typename) is not valid, where
         * non-std gcc extension might return 0, or 1 of an empty
         * struct. All copy_from/to etc. operations on this type
         * just returns a pointer without using sizeof.
         *
         * We ought to define size as a define so it can be used in a
         * switch, but that does not mesth with flatcc_accessors.h
         * macros, so we use an inline function. Users would normally
         * use sizeof which will break for empty which is ok, and
         * internal operations can use size() where generic behavior is
         * required.
         */
        fprintf(out->fp, "/* empty struct already typedef'ed as void since this not permitted in std. C: struct %s {}; */\n", snt.text);
        fprintf(out->fp,
                "static inline const %s_t *%s__const_ptr_add(const %s_t *p, size_t i) { return p; }\n", snt.text, snt.text, snt.text);
        fprintf(out->fp,
                "static inline %s_t *%s__ptr_add(%s_t *p, size_t i) { return p; }\n", snt.text, snt.text, snt.text);
        fprintf(out->fp,
                "static inline %s_struct_t %s_vec_at(%s_vec_t vec, size_t i) { return vec; }\n", snt.text, snt.text, snt.text);
    } else {
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
            if (do_pad && (pad = (unsigned)(member->offset - offset))) {
                fprintf(out->fp, "    uint8_t __padding%u[%u];\n",
                        pad_index++, pad);
            }
            if (member->metadata_flags & fb_f_deprecated) {
                pad = (unsigned)member->size;
                if (do_pad) {
                    fprintf(out->fp, "    uint8_t __deprecated%u[%u]; /* was: '%.*s' */\n",
                            deprecated_index++, pad, n, s);
                } else {
                    fprintf(out->fp, "    alignas(%u) uint8_t __deprecated%u[%u]; /* was: '%.*s' */\n",
                            align, deprecated_index++, pad, n, s);
                }
                offset = (unsigned)(member->offset + member->size);
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
            offset = (unsigned)(member->offset + member->size);
        }
        if (do_pad && (pad = (unsigned)(ct->size - offset))) {
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
                "static inline const %s_t *%s__const_ptr_add(const %s_t *p, size_t i) { return p + i; }\n", snt.text, snt.text, snt.text);
        fprintf(out->fp,
                "static inline %s_t *%s__ptr_add(%s_t *p, size_t i) { return p + i; }\n", snt.text, snt.text, snt.text);
        fprintf(out->fp,
                "static inline %s_struct_t %s_vec_at(%s_vec_t vec, size_t i)\n"
                "__%sstruct_vec_at(vec, i)\n",
                snt.text, snt.text, snt.text,
                nsc);
    }
    fprintf(out->fp, "static inline size_t %s__size() { return %llu; }\n",
            snt.text, llu(ct->size));
    print_type_identifier(out, snt.text, ct->type_hash);
    fprintf(out->fp,
            "static inline size_t %s_vec_len(%s_vec_t vec)\n"
            "__%svec_len(vec)\n",
            snt.text, snt.text, nsc);
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
                "__%sstruct_scalar_field(t, %.*s, %s%s)\n",
                tname_ns, tname, snt.text, n, s, snt.text,
                nsc, n, s, nsc, tname_prefix);
            if (out->opts->allow_scan_for_all_fields || (member->metadata_flags & fb_f_key)) {
                fprintf(out->fp,
                        "__%sdefine_scan_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
            }
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
                        "__%sdefine_default_find_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
                    fprintf(out->fp,
                        "__%sdefine_default_scan_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
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
                    "__%sstruct_scalar_field(t, %.*s, %s%s)\n",
                    snref.text, snt.text, n, s, snt.text,
                    nsc, n, s, nsc, tname_prefix);
                if (out->opts->allow_scan_for_all_fields || (member->metadata_flags & fb_f_key)) {
                    fprintf(out->fp,
                            "__%sdefine_scan_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
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
                            "__%sdefine_sort_by_scalar_field(%s, %.*s, %s_enum_t, %s_t)\n",
                            nsc, snt.text, n, s, snref.text, snt.text);
                    }
                    if (!already_has_key) {
                        fprintf(out->fp,
                            "__%sdefine_default_find_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
                        fprintf(out->fp,
                            "__%sdefine_default_scan_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
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
                    "__%sstruct_struct_field(t, %.*s)\n",
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
static void gen_enum(fb_output_t *out, fb_compound_type_t *ct)
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

    w = (int)ct->size * 8;

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
        /*
         * This must be a define, not a static const integer, otherwise it
         * won't work in switch statements - except with GNU extensions.
         */
        switch (member->value.type) {
        case vt_uint:
            fprintf(out->fp,
                    "#define %s_%.*s ((%s_%s_t)%llu%s)\n",
                    snt.text, n, s, snt.text, kind, llu(member->value.u), suffix);
            break;
        case vt_int:
            fprintf(out->fp,
                    "#define %s_%.*s ((%s_%s_t)%lld%s)\n",
                    snt.text, n, s, snt.text, kind, lld(member->value.i), suffix);
            break;
        case vt_bool:
            fprintf(out->fp,
                    "#define %s_%.*s ((%s_%s_t)%u)\n",
                    snt.text, n, s, snt.text, kind, member->value.b);
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
            fprintf(out->fp,
                    "    /* case %s_%.*s: return \"%.*s\"; (duplicate) */\n",
                    snt.text, n, s, n, s);
        } else {
            fprintf(out->fp,
                    "    case %s_%.*s: return \"%.*s\";\n",
                    snt.text, n, s, n, s);
        }
    }
    fprintf(out->fp,
            "    default: return \"\";\n"
            "    }\n"
            "}\n");
    fprintf(out->fp, "\n");
}

static void gen_nested_root(fb_output_t *out, fb_symbol_t *root_type, fb_symbol_t *container, fb_symbol_t *member)
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

static void gen_table(fb_output_t *out, fb_compound_type_t *ct)
{
    fb_member_t *member;
    fb_symbol_t *sym;
    const char *s, *tname, *tname_ns, *tname_prefix;
    int n, r;
    int already_has_key, current_key_processed, has_is_present;
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
    print_type_identifier(out, snt.text, ct->type_hash);
    fprintf(out->fp,
            "static inline size_t %s_vec_len(%s_vec_t vec)\n"
            "__%svec_len(vec)\n",
            snt.text, snt.text, nsc);
    fprintf(out->fp,
            "static inline %s_table_t %s_vec_at(%s_vec_t vec, size_t i)\n"
            "__%soffset_vec_at(%s_table_t, vec, i, 0)\n",
            snt.text, snt.text, snt.text, nsc, snt.text);
    fprintf(out->fp,
            "__%stable_as_root(%s)\n",
            nsc, snt.text);
    fprintf(out->fp, "\n");

    already_has_key = 0;
    for (sym = ct->members; sym; sym = sym->link) {
        has_is_present = 0;
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
            has_is_present = 1;
            tname_ns = scalar_type_ns(member->type.st, nsc);
            tname = scalar_type_name(member->type.st);
            tname_prefix = scalar_type_prefix(member->type.st);
            switch (member->value.type) {
            case vt_uint:
                fprintf(out->fp,
                    "__%sdefine_scalar_field(%llu, %s, %.*s, %s%s, %s%s, %llu)\n",
                    nsc, llu(member->id), snt.text, n, s, nsc, tname_prefix, tname_ns, tname, llu(member->value.u));
                break;
            case vt_int:
                fprintf(out->fp,
                    "__%sdefine_scalar_field(%llu, %s, %.*s, %s%s, %s%s, %llu)\n",
                    nsc, llu(member->id), snt.text, n, s, nsc, tname_prefix, tname_ns, tname, llu(member->value.u));
                break;
            case vt_bool:
                fprintf(out->fp,
                    "__%sdefine_scalar_field(%llu, %s, %.*s, %s%s, %s%s, %llu)\n",
                    nsc, llu(member->id), snt.text, n, s, nsc, tname_prefix, tname_ns, tname, llu(member->value.u));
                break;
            case vt_float:
                fprintf(out->fp,
                    "__%sdefine_scalar_field(%llu, %s, %.*s, %s%s, %s%s, %llu)\n",
                    nsc, llu(member->id), snt.text, n, s, nsc, tname_prefix, tname_ns, tname, llu(member->value.u));
                break;
            default:
                gen_panic(out, "internal error: unexpected scalar table default value");
                continue;
            }
            if (!out->opts->allow_scan_for_all_fields && (member->metadata_flags & fb_f_key)) {
                fprintf(out->fp,
                        "__%sdefine_scan_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
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
                        "__%sdefine_default_find_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
                    fprintf(out->fp,
                        "__%sdefine_default_scan_by_scalar_field(%s, %.*s, %s%s)\n",
                        nsc, snt.text, n, s, tname_ns, tname);
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
            if (out->opts->allow_scan_for_all_fields || (member->metadata_flags & fb_f_key)) {
                fprintf(out->fp,
                    "__%sdefine_scan_by_string_field(%s, %.*s)\n",
                    nsc, snt.text, n, s);
            }
            if (member->metadata_flags & fb_f_key) {
                if (already_has_key) {
                    fprintf(out->fp, "/* Note: this is not the first field with a key on this table. */\n");
                }
                fprintf(out->fp,
                    "__%sdefine_find_by_string_field(%s, %.*s)\n",
                    nsc, snt.text, n, s);
                if (out->opts->cgen_sort) {
                    fprintf(out->fp,
                        "__%sdefine_sort_by_string_field(%s, %.*s)\n",
                        nsc, snt.text, n, s);
                }
                if (!already_has_key) {
                    fprintf(out->fp,
                        "__%sdefine_default_find_by_string_field(%s, %.*s)\n",
                        nsc, snt.text, n, s);
                    fprintf(out->fp,
                        "__%sdefine_default_scan_by_string_field(%s, %.*s)\n",
                        nsc, snt.text, n, s);
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
                has_is_present = 1;
                switch (member->value.type) {
                case vt_uint:
                    fprintf(out->fp,
                        "__%sdefine_scalar_field(%llu, %s, %.*s, %s, %s_enum_t, %llu)\n",
                        nsc, llu(member->id), snt.text, n, s, snref.text, snref.text, llu(member->value.u));
                    break;
                case vt_int:
                    fprintf(out->fp,
                        "__%sdefine_scalar_field(%llu, %s, %.*s, %s, %s_enum_t, %lld)\n",
                        nsc, llu(member->id), snt.text, n, s, snref.text, snref.text, lld(member->value.i));
                    break;
                case vt_bool:
                    fprintf(out->fp,
                        "__%sdefine_scalar_field(%llu, %s, %.*s, %s, %s_enum_t, %u)\n",
                        nsc, llu(member->id), snt.text, n, s, snref.text, snref.text, member->value.b);
                    break;
                default:
                    gen_panic(out, "internal error: unexpected enum type referenced by table");
                    continue;
                }
                if (!out->opts->allow_scan_for_all_fields && (member->metadata_flags & fb_f_key)) {
                    fprintf(out->fp,
                            "__%sdefine_scan_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                            nsc, snt.text, n, s, snref.text);
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
                                "__%sdefine_default_find_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                                nsc, snt.text, n, s, snref.text);
                        fprintf(out->fp,
                                "__%sdefine_default_scan_by_scalar_field(%s, %.*s, %s_enum_t)\n",
                                nsc, snt.text, n, s, snref.text);
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
                    "__%sunion_type_field(%s, %llu, 0, t)\n",
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
        if (!has_is_present) {
            fprintf(out->fp,
                    "static inline int %s_%.*s_is_present(%s_table_t t)\n"
                    "__%sfield_present(%llu, t)\n",
                    snt.text, n, s, snt.text, nsc, llu(present_id));
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

int fb_gen_c_reader(fb_output_t *out)
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
        case fb_is_rpc_service:
            /* Ignore. */
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
