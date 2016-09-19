#include "codegen_c_sort.h"

/*
 * We choose heapsort because it is about as fast as quicksort, avoids
 * recursion, the code is compact which makes it practical to specialize for
 * different vector types, it can sort the flatbuffer arrays in-place,
 * and it has only a few places with comparisons. Furthermore, heapsort
 * has worst case (n log n) upperbound where quicksort has O(n^2) which
 * is an attack vector, and could be a problem with large datasets
 * The sort is not stable.
 *
 * Some arguments are similar to those of the __%sfind_by_field macro.
 *
 * NS: The namespace
 * N: the name of the vector type
 * X: the name suffix when there are multiple sorts for same vector type.
 * E: Element accessor (elem = E(vector, index)).
 * L: Vector length.
 * A: Field accessor (or the identity function), result must match the diff function D.
 * TK: The scalar, enum or string key type, (either the element, or a field of the element).
 * TE: The raw element type - uoffset_t for tables and strings.
 * for swap.
 * D: The diff function, but unlike __find_by_field, the second
 *    argument is returned by A, not a search key, and there is no third argument.
 * S: Swap operation - must handle offset change when offset elements are moved.
 */

int gen_sort(fb_output_t *out)
{
    fprintf(out->fp,
        "#define __%sheap_sort(N, X, A, E, L, TK, TE, D, S)\\\n"
        "static inline void __ ## N ## X ## __heap_sift_down(\\\n"
        "        N ## _mutable_vec_t vec, size_t start, size_t end)\\\n"
        "{ size_t child, root; TK v1, v2, vroot;\\\n"
        "  root = start;\\\n"
        "  while ((root << 1) <= end) {\\\n"
        "    child = root << 1;\\\n"
        "    if (child < end) {\\\n"
        "      v1 = A(E(vec, child));\\\n"
        "      v2 = A(E(vec, child + 1));\\\n"
        "      if (D(v1, v2) < 0) {\\\n"
        "        child++;\\\n"
        "      }\\\n"
        "    }\\\n"
        "    vroot = A(E(vec, root));\\\n"
        "    v1 = A(E(vec, child));\\\n"
        "    if (D(vroot, v1) < 0) {\\\n"
        "      S(vec, root, child, TE);\\\n"
        "      root = child;\\\n"
        "    } else {\\\n"
        "      return;\\\n"
        "    }\\\n"
        "  }\\\n"
        "}\\\n"
        "static inline void __ ## N ## X ## __heap_sort(N ## _mutable_vec_t vec)\\\n"
        "{ size_t start, end, size;\\\n"
        "  size = L(vec); if (size == 0) return; end = size - 1; start = size >> 1;\\\n"
        "  do { __ ## N ## X ## __heap_sift_down(vec, start, end); } while (start--);\\\n"
        "  while (end > 0) { \\\n"
        "    S(vec, 0, end, TE);\\\n"
        "    __ ## N ## X ## __heap_sift_down(vec, 0, --end); } }\n",
        out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_sort_by_field(N, NK, TK, TE, D, S)\\\n"
        "  __%sheap_sort(N, _sort_by_ ## NK, N ## _ ## NK, N ## _vec_at, N ## _vec_len, TK, TE, D, S)\\\n"
        "static inline void N ## _vec_sort_by_ ## NK(N ## _mutable_vec_t vec)\\\n"
        "{ __ ## N ## _sort_by_ ## NK ## __heap_sort(vec); }\n",
        out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_sort(N, TK, TE, D, S)\\\n"
        "__%sheap_sort(N, , __%sidentity, N ## _vec_at, N ## _vec_len, TK, TE, D, S)\\\n"
        "static inline void N ## _vec_sort(N ## _mutable_vec_t vec) { __ ## N ## __heap_sort(vec); }\n",
        out->nsc, out->nsc, out->nsc);
    fprintf(out->fp,
        /* Subtractions doesn't work for unsigned types. */
        "#define __%sscalar_diff(x, y) ((x) < (y) ? -1 : (x) > (y))\n"
        "#define __%sstring_diff(x, y) __%sstring_n_cmp((x), (const char *)(y), %sstring_len(y))\n",
        out->nsc, out->nsc, out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sscalar_swap(vec, a, b, TE) { TE tmp = vec[b]; vec[b] = vec[a]; vec[a] = tmp; }\n"
        "#define __%sstring_swap(vec, a, b, TE)\\\n"
        "{ TE tmp, d; d = (TE)((a - b) * sizeof(vec[0])); tmp = vec[b]; vec[b] = vec[a] + d; vec[a] = tmp - d; }\n",
        out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_sort_by_scalar_field(N, NK, TK, TE)\\\n"
        "  __%sdefine_sort_by_field(N, NK, TK, TE, __%sscalar_diff, __%sscalar_swap)\n",
        out->nsc, out->nsc, out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_sort_by_string_field(N, NK)\\\n"
        "  __%sdefine_sort_by_field(N, NK, %sstring_t, %suoffset_t, __%sstring_diff, __%sstring_swap)\n",
        out->nsc, out->nsc, out->nsc, out->nsc, out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_scalar_sort(N, T) __%sdefine_sort(N, T, T, __%sscalar_diff, __%sscalar_swap)\n",
        out->nsc, out->nsc, out->nsc, out->nsc);
    fprintf(out->fp,
        "#define __%sdefine_string_sort() __%sdefine_sort(%sstring, %sstring_t, %suoffset_t, __%sstring_diff, __%sstring_swap)\n",
        out->nsc, out->nsc, out->nsc, out->nsc, out->nsc, out->nsc, out->nsc);
    return 0;
}

/* reference implementation */
#if 0

/* from github swenson/sort */
/* heap sort: based on wikipedia */
static __inline void HEAP_SIFT_DOWN(SORT_TYPE *dst, const int64_t start, const int64_t end) {
  int64_t root = start;

  while ((root << 1) <= end) {
    int64_t child = root << 1;

    if ((child < end) && (SORT_CMP(dst[child], dst[child + 1]) < 0)) {
      child++;
    }

    if (SORT_CMP(dst[root], dst[child]) < 0) {
      SORT_SWAP(dst[root], dst[child]);
      root = child;
    } else {
      return;
    }
  }
}

static __inline void HEAPIFY(SORT_TYPE *dst, const size_t size) {
  int64_t start = size >> 1;

  while (start >= 0) {
    HEAP_SIFT_DOWN(dst, start, size - 1);
    start--;
  }
}

void HEAP_SORT(SORT_TYPE *dst, const size_t size) {
  /* don't bother sorting an array of size 0 */
  if (size == 0) {
    return;
  }

  int64_t end = size - 1;
  HEAPIFY(dst, size);

  while (end > 0) {
    SORT_SWAP(dst[end], dst[0]);
    HEAP_SIFT_DOWN(dst, 0, end - 1);
    end--;
  }
}

#endif
