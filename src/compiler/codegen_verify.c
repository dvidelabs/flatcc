/*
 * Experimental support for verifying buffers.
 */

/*
 * IMPORTANT: verification will make it safe to read from a previously
 * untruested buffer. However, it will not make it safe to update it
 * in-place. Setting a scalar may harm later reads and sorting may
 * crash. This is because verification does not ensure datastructures
 * do not overlap.
 *
 * Verification checks that an offset from a valid buffer location
 * refers to a new valid location including the size and alignment of
 * the new location. Offsets may be soffsets to vtables, voffsets from
 * within vtables, or uoffsets from within tables to other objects, or
 * buffer start offset. Vectors, strings, and vtables are also range
 * checked. Tables are optionally range checked - but not required as
 * any reachable table content is checked via vtable offsets. Alignment
 * checks may be skipped on some platforms.
 *
 * All verify operations assume the buffer starts at zero and that `end`
 * is the buffer size in bytes. Some operations take a `buf` pointer
 * argument such that `base` andn `end` are relative to `buf` when
 * buffer reads are required.
 *
 * `base` is generally a value previously tested so `0 <= base <= end`
 * and such that `base` is aligned to whatever is relevant.
 *
 * Checks are generally made as offset to a base because checking the
 * sum directly provides less opportunity for overflow checking.
 */

enum vt_type {
    vt_None = 0,
    vt_UType = 1,
    vt_Bool = 2,
    vt_Byte = 3,
    vt_UByte = 4,
    vt_Short = 5,
    vt_UShort = 6,
    vt_Int = 7,
    vt_UInt = 8,
    vt_Long = 9,
    vt_ULong = 10,
    vt_Float = 11,
    vt_Double = 12,
    vt_String = 13,
    vt_Vector = 14,
    /* Struct or Table */
    vt_Obj = 15,
    vt_Union = 16,
    /*
     * Externsion to reflection schema so we can tell structs from
     * tables, and identify table and string vectors.
     */
    vt_IsString = 64, /* set with vt_Vector */
    vt_IsTable = 128, /* set with vt_Obj or vt_Vector */
};

typedef struct vt_entry vt_entry_t;

/* Stored as array for each table type and as single element for structs. */
struct vt_entry {
    voffset_t size;
    uint8_t align_mask; /* align - 1 */
    uint8_t type;
};

typedef int table_verifier_fun(uint8_t *buf, uoffset_t base, uoffset_t end, vt_cache_entry_t *vt_cache);

struct table_verifier {
    uint8_t *buf;
    uoffset_t end;
    vt_entry_t *entries;
    voffset_t nentries;

    vt_cache_entry_t vt_cache[VT_CACHE_SIZE];
};

static inline is_aligned(uoffset_t x, short align)
{
    return (x & (align -1)) == 0;
}

/*
 * We trust base, but not base + soff or base + soff + size including
 * not trusting alignment of soff but trust size is valid.
 */
static inline int verify_srange(uoffset_t base, uoffset_end, soffset_t soff, uoffset_t size, short align)
{
    soffset_t k1 = base + soff;
    soffset_t k2 = k1 + size;
    return k1 >= 0 && is_aligned((uoffset_t)k1, align) && k2 >= k && k2 <= end;
}

/*
 * We trust `base`, but not `base + uoff` or `base + uoff + size` including
 * not trusting alignment of `uoff` but trust that size is valid.
 */
static inline int verify_urange(uoffset_t base, uoffset_end, uoffset_t uoff, uoffset_t size, short align)
{
    uoffset_t k1 = base + uoff;
    uoffset_t k2 = k1 + size;
    return k1 >= base && is_aligned(k1, align) && k2 >= k1 && k2 <= end;
}

/* We trust `base` but not but not `base + size`, and not that `size` is aligned. */
static inline int verify_size(uoffset_t base, uoffset_end, uoffset_t size, short align)
{
    uoffset_t k = base + size;
    return is_aligned(size, align) && k >= base && k <= end;
}

/* We trust `base` and that `base` and `size` are aligned, but not `base + size <= end`. */
static inline int verify_width(uoffset_t base, uoffse_t end, uoffset_t size)
{
    uoffset_t k = base + size;
    return k >= base && k <= end;
}

/* This assumes the table base and size have been verified. */
static inline int verify_field(uoffset_t base, voffset_t table_size, voffset_t field_offset, voffset_t field_size, short align)
{
    return verify_urange(base, base + table_size, field_offset, field_size, align);
}

static inline int verify_scalar_field(uoffset_t base, voffset_t table_size, voffset_t field_offset, voffset_t field_size)
{
    return verify_field(base, table_size, field_offset, field_size, field_size);
}

static inline int verify_struct_field(uoffset_t base, voffset_t table_size, voffset_t field_offset, voffset_t field_size)
{
    return verify_field(base, table_size, field_offset, field_size, field_size);
}

static inline int verify_offset_field(uoffset_t base, voffset_t table_size, voffset_t field_offset)
{
    return verify_field(base, table_size, field_offset, sizeof(uoffset_t), sizeof(uoffet_t));
}

static inline int verify_string_field(uint8_t *buf, uoffset_t base, uoffset_t end, voffset_t table_size, voffset_t field_offset, voffset_t field_size)
{
    uint8_t *p;
    uoffset_t uoff;

    if (!verify_offset_field(base, table_size, field_offset)) {
        return 0;
    }
    p = buf + base + field_offset;
    uoff = read_uoffset_ptr(p);
    return verify_string(buf, base, end, uoff);
}

typedef int vt_verifier_fun(uint8_t *buf, uoffset_t base, uoffset_t end, voffset_t *vt, voffset_t vt_size, vt_cache_entry_t *vt_cache);

#define vt_entry_size(x) (sizeof(x)/sizeof(x[0]))
vt_entry_t __sample_1_vt_entry[] = { };
vt_entry_t __sample_2_vt_entry[] = { { 4, 4, vt_Int }, { 8, 4, vt_Obj }, { 0, 0, vt_Obj | vt_IsTable } };
static int __sample_1_verifier_1(uint8_t *buf, uoffset_t base, uoffset_t end, voffset_t *vt, voffset_t vt_size, vt_cache_entry_t *vt_cache)
{
    return 0;
}
static int __sample_2_verifier(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff, vt_cache_entry_t *vt_cache, vt_verifier_fun *_this)
{
    voffset_t voff;
    uint8_t *t;
    int n = sizeof(__sample_vt_entry_2)/sizeof(__sample_vt_entry_2[0]);

    if (!verify_table_body(buf, base, end, uoff, vt_cache, __sample_vt_entry_2, n, _this)) {
        return 0;
    }
    while (n--) {
        voff = read_voffset_ptr(vt);
        ++vt;
    }


}

#define VT_CACHE_SIZE 8
#ifdef FLATCC_SLOW_MUL
/*
 * Worst case we just cache the last good vtable, which is much better
 * than doing nothing.
 */
#define VT_CACHE_INDEX(base) (((base) >> 2) & (VT_CACHE_SIZE - 1))
#else
#define VT_CACHE_INDEX(base) (((base) * 0x2f693b52) & (VT_CACHE_SIZE - 1))
#endif
typedef struct vt_cache_entry {
    uoffset_t vbase;
    voffset_t t_size;
    vt_verifier_fun *vf;
} vt_cache_entry_t;

/*
 * cache_vt is a simple cache that only remembers the last verified
 * vtable. It is useful for vectors of like tables.
 * The cache could easily be extended to an n-way cache.
 *
 * NOTE: we cannot assume that a tables inline content is good if the
 * cached vtable is good because vtables are shared across vtable types.
 * The cache is therefore only accepted if the vtable and verifier
 * function both matches. It is not likely to have a vtable match
 * without a verifier function match so it is not needed in the hash.
 */
static int verify_table(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff, vt_cache_entry_t *vt_cache, vt_verifier_fun *vf)
{
    uoffset_t end = tv->end;
    soffset_t soff;
    voffset_t *vt, vt_size, i, voff, vbase;
    uint8_t *t;
    int h;

    if (!verify_urange(base, end, uoff, sizeof(soffset_t), sizeof(soffset_t))) {
        assert(0 && "vector header field not accessible");
        return 0;
    }
    base += uoff;
    t = tv->buf + base;
    soff = (soffset_t)read_uoffset_ptr(t);
    if (soff == *cache_vt) {
        return 1;
    }
    vbase = base + soff;
    h = VT_CACHE_INDEX(vbase);
    if (vbase == vt_cache[h].vbase && vt_cache[h].entries == entries) {
        /* vtable is good, but we must still check the table size. */
        if (!verify_width(base, end, vt_cache[h].t_size)) {
            assert(0 && "invalid table size");
            return 0;
        }
        /* Now offset / union fields remain to be checked. */
        return 1;
    }
    if (!verify_srange(base, end, soff, 2 * sizeof(voffset_t), sizeof(voffset_t))) {
        assert(0 && "invalid vtable header location");
        return 0;
    }
    vt = (voffset_t *)(t + soff);
    vt_size = read_voffset_ptr(vt);
    if (!verify_size(base + soff, end, vt_size, sizeof(voffset_t))) {
        assert(0 && "invalid vtable size");
        return 0;
    }
    t_size = read_voffset_ptr(vt + 1);
    if (!verify_width(base, end, t_size)) {
        assert(0 && "invalid table size");
        return 0;
    }
    /* Mostly relevant if voffset_t has a size > 2. */
#if MAX_VTABLE_SIZE
    if (vt_size > MAX_VTABLE_SIZE) {
        asssert(0 && "vtable size too large");
        return 0;
    }
#endif
    if ((entry_count + 2) * sizeof(voffset_t) > vt_size) {
        entry_count = vt_size / sizeof(voffset_t) - 2;
    }
    vt += 2;
    /* This does not check for required offset fields. */
    for (i = 0; i < entry_count; ++i) {
        voff = read_voffset_ptr(vt + i);
        if (voff && !verify(base, base + t_size, (uoffset_t)voff, entries[i].size, entries[i].align)) {
            assert(0 &&  "invalid vtable entry");
            return 0;
        }
    }
    /*
     * Now it is safe to read content in the table. A type specific
     * function should recurse over offset fields.
     */
    /* Overwrite existing entry if any, MRU style. */
    vt_cache[h] = { vbase, t_size, entries };
    return 1;
}

/* `align` may be 1 for any alignment <= sizeof(uoffset_t) to optimize better. */
static inline int verify_vector_body(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff, uoffset_t elem_size, short align, int is_string)
{
    uint8_t *v;
    uoffset_t vec_len;
    /* Must be 64 bit to handle overflows. */
    uint64_t size, k1, k2;

    v = buf + base + uoff;
    vec_len = read_uoffset_ptr(v);
    /*
     * Note: for zero size elements, any count is valid. This may be used
     * in a DOS attack by creating very large vector counts but we do not
     * fail this case by default. MAX_VEC_LEN can be set to facility this
     * check for zero and other sized elements. The setting is in number
     * of elements, not in bytes.
     */
#if MAX_VEC_LEN
    if (vec_len > MAX_VEC_LEN) {
        assert(0 && "maximum allowed vector length exceed");
        return 0;
    }
#endif
    size = vec_len * elem_size;
    /* Verify urange could be used, but we have more knowledge. */
    k1 = base + uoff + sizeof(uoffset_t);
    k2 = k1 + size + (is_string != 0);

    /* We already know k1 is not overflowing due to urange check earlier. */
    if (!(is_aligned((uoffset_t)k1, align) && k2 >= k1 && k2 <= end)) {
        assert(0 && "vector too large");
        return 0;
    }
    if (is_string && *(buf + k2 - 1)) {
        assert(0 && "string not null terminated");
        return 0;
    }

    /*
     * The vec_len * elem_size math may overflow such that the end is
     * valid but the middle isn't. This can be caused by a careless
     * buffer generator, or by exploit.
     *
     * Range checks suffice when doing math in a larger domain, but if
     * we cannot do that, we must check using division. Hopefully the
     * `elem_size` is usually a constant and small power of two.
     *
     * Check for elem_size == 1 also accounts for is_string adjustment
     * to k2.
     */
    if (sizeof(uoffset_t) <= 4 || elem_size == 1) {
        return 1;
    }
    /* Structs can have zero size. Any count is valid. */
    if (elem_size == 0) {
        return 1;
    }
    if ((k2 - k1) / elem_size != vec_len) {
        assert(0 && "vector count math overflow");
        return 0;
    }
    /* The vector is now safe to access. */
    return 1;
}

static inline int verify_header(uoffset_t base, uoffset_t end, uoffset_t uoff)
{
    if (!verify_urange(base, end, uoff, sizeof(uoffset_t), sizeof(uoffset_t))) {
        assert(0 && "header field not accessible");
        return 0;
    }
}

static inline int verify_vector(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff, uoffset_t elem_size, short align, int is_string)
{
    return verify_header(base, end, uoff) &&
        verify_vector_body(buf, base, end, uoff, elem_size, align, is_string);
}

/*
 * Does not verify the vector contents but is a precondition to
 * accessing it.
 */
static inline int verify_offset_vector(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff)
{
    return verify_vector(buf, base, end, uoff, sizeof(uoffset_t), 1, 0);
}

static inline int verify_string(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff)
{
    return verify_vector(buf, base, end, uoff, 1, 1, 1);
}

static inline int verify_string_vector(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff)
{
    uoffset_t vec_len;
    uint8_t *hdr;

    if (!verify_offset_vector(buf, base, end, uoff) {
        return 0;
    }
    hdr = (buf + base + uoff);
    vec_len = read_offset_ptr(hdr);
    base = base + uoff + sizeof(uoffset_t);
    while (vec_len--) {
        uoff = read_uoffset_ptr(buf + base);
        if (!verify_string(buf, base, uoff) {
            return 0;
        }
        base += sizeof(uoffset_t);
    }
    return 1;
}

static int verify_table_vector(uint8_t *buf, uoffset_t base, uoffset_t end, uoffset_t uoff, vt_cache_t *vt_cache, vt_verifier_fun *vf)
{
    uoffset_t vec_len, end = tv->end;
    uint8_t *hdr;

    if (!verify_offset_vector(buf, base, end, uoff) {
        return 0;
    }
    hdr = (buf + base + uoff);
    vec_len = read_offset_ptr(hdr);
    base = base + uoff + sizeof(uoffset_t);
    while (vec_len--) {
        uoff = read_uoffset_ptr(buf + base);
        if (!verify_table(buf, base, end, uoff, vt_cache, vf) {
            return 0;
        }
        base += sizeof(uoffset_t);
    }
    return 1;
}

#define IDENTIFIER_SIZE 4
/*
 * Buffers need not be aligned to 8 in all cases, but it othewise
 * greatly complicates verifying that aligned reads are safe.
 */
#define MIN_BUF_ALIGN 8


static inline int verify_buffer_header(uint8_t *buf, uoffset_t end, const char *identifier)
{
    if (end < sizeof(uoffset) + IDENTIFIER_SIZE) {
        /*
         * Even if the buffer does not have an identifer, it must be
         * large enough to check an identifier. Struct buffers could
         * theoretically have a size of uoffset_t otherwise, given an
         * empty struct.
         */
        assert(0 && "buffer too small to access");
        return 0;
    }
    if (!is_aligned((uoffset_t)buf, MIN_BUF_ALIGN)) {
        assert(0 && "buffer pointer not aligned");
        return 0;
    }
    if (identifier) {
        if (memcmp(buf + sizeof(uoffset_t), identifier, IDENTIFIER_SIZE)) {
            assert(0 && "identifier mismatch");
            return 0;
        }
    }
    return 1;
}

/* `identifier` may be null. `end` is buffer size. */
int flatcc_verify_root_struct(uint8_t *buf, uoffset_t end, const char *identifier, uoffset_t struct_size, short struct_align)
{
    uoffset_t uoff;

    if (!verify_buffer_header(buf, end, identifier)) {
        return 0;
    }
    uoff = read_uoffset_ptr(buf);
    if (!verify_urange(0, end, uoff, struct_size, struct_align)) {
        assert(0 && "invalid root struct offset");
        return 0;
    }
    /* Now it is safe to access the struct. */
    return 1;
}

/*
 * `identifier` may be null. `end` is buffer size and `vf` is the
 * expected root table types verifier function.
 */
int flatcc_verify_root_table(uint8_t *buf, uoffset_t end, const char *identifier, vt_verifier_fun *vf)
{
    vt_cache_entry_t vt_cache[VT_CACHE_SIZE];
    uoffset_t uoff;

    memset(&vt_cache, 0, sizeof(vt_cache));
    if (!verify_buffer_header(buf, end, identifier)) {
        return 0;
    }
    uoff = read_uoffset_ptr(buf);
    /* `uoffset_t` points to table, `soffset_t` is the header field of the table. */
    if (!verify_urange(0, end, uoff, sizeof(soffset_t), sizeof(soffset_t))) {
        assert(0 && "invalid root table offset");
    }
    return verify_table(buf, 0, end, uoff, &vt_cache, vf);
}
