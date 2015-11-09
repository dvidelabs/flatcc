/*
 * Codegenerator for C, building FlatBuffers.
 *
 * There are several approaches, some light, some requiring a library,
 * some with vectored I/O etc.
 *
 * Here we focus on a reasonable balance of light code and efficiency.
 *
 * Builder code is generated to a separate file that includes the
 * generated read-only code.
 *
 * Mutable buffers are not supported in this version.
 *
 */

#include <memory.h>
#include <string.h>
#include <assert.h>

#include "flatcc/flatcc_builder.h"
#include "flatcc/flatcc_endian.h"
#include "flatcc/flatcc_emitter.h"

/* `strnlen` not widely supported. */
static inline size_t pstrnlen(const char *s, size_t max_len)
{
    const char *end = memchr (s, 0, max_len);
    return end ? (size_t)(end - s) : max_len;
}
#undef strnlen
#define strnlen pstrnlen

/* `align` must be a power of 2. */
#define alignup(x, align) (((x) + (align) - 1) & ~((align) - 1))

/* Padding can be up to 255 zeroes, and 1 zero string termination byte.
 * When two paddings are combined at nested buffers, we need twice that.
 * Visible to emitter so it can test for zero padding in iov. */
const uint8_t flatcc_builder_padding_base[512] = { 0 };
#define _pad flatcc_builder_padding_base

#define uoffset_t flatbuffers_uoffset_t
#define soffset_t flatbuffers_soffset_t
#define voffset_t flatbuffers_voffset_t

#define store_uoffset flatbuffers_store_uoffset
#define store_voffset flatbuffers_store_voffset

#define field_size sizeof(uoffset_t)
#define max_offset_count FLATBUFFERS_COUNT_MAX(field_size)
#define max_string_len FLATBUFFERS_COUNT_MAX(1)
#define identifier_size FLATBUFFERS_IDENTIFIER_SIZE

#define iovec_t flatcc_iovec_t
#define frame_size sizeof(__flatcc_builder_frame_t)
#define frame(x) (B->frame[0].x)

typedef struct vtable_descriptor vtable_descriptor_t;
struct vtable_descriptor {
    /* Where the vtable is emitted. */
    flatcc_builder_ref_t vt_ref;
    /* Which buffer it was emitted to. */
    flatcc_builder_ref_t buffer_mark;
    /* Where the vtable is cached. */
    uoffset_t vb_start;
    /* Hash table collision chain. */
    uoffset_t next;
};

typedef struct flatcc_iov_state flatcc_iov_state_t;
struct flatcc_iov_state {
    size_t len;
    size_t count;
    flatcc_iovec_t iov[FLATCC_IOV_COUNT_MAX];
};

#define iov_state_t flatcc_iov_state_t

/* This assumes `iov_state_t iov;` has been declared in scope */
#define push_iov_cond(base, size, cond) if ((size) > 0 && (cond)) { iov.len += size;\
        iov.iov[iov.count].iov_base = (void *)(base); iov.iov[iov.count].iov_len = (size); ++iov.count; }
#define push_iov(base, size) push_iov_cond(base, size, 1)
#define init_iov() { iov.len = 0; iov.count = 0; }


int flatcc_builder_default_alloc(void *alloc_context, iovec_t *b, size_t request, int zero_fill, int hint)
{
    void *p;
    size_t n;

    (void)alloc_context;

    if (request == 0) {
        if (b->iov_base) {
            free(b->iov_base);
            b->iov_base = 0;
            b->iov_len = 0;
        }
        return 0;
    }
    switch (hint) {
    case flatcc_builder_alloc_ds:
        n = 256;
        break;
    case flatcc_builder_alloc_ht:
        /* Should be exact size, or space size is just wasted. */
        n = request;
        break;
    case flatcc_builder_alloc_fs:
        n = sizeof(__flatcc_builder_frame_t) * 8;
        break;
    default:
        /*
         * We have many small structures - vs stack for tables with few
         * elements, and few offset fields in patch log. No need to
         * overallocate in case of busy small messages.
         */
        n = 32;
        break;
    }
    while (n < request) {
        n *= 2;
    }
    if (request <= b->iov_len && b->iov_len / 2 >= n) {
        /* Add hysteresis to shrink. */
        return 0;
    }
    if (!(p = realloc(b->iov_base, n))) {
        return -1;
    }
    /* Realloc might also shrink. */
    if (zero_fill && b->iov_len < n) {
        memset((uint8_t *)p + b->iov_len, 0, n - b->iov_len);
    }
    b->iov_base = p;
    b->iov_len = n;
    return 0;
}

#define T_ptr(base, pos) ((void *)((uint8_t *)(base) + (uoffset_t)(pos)))
#define ds_ptr(pos) (T_ptr(B->buffers[flatcc_builder_alloc_ds].iov_base, (pos)))
#define vs_ptr(pos) (T_ptr(B->buffers[flatcc_builder_alloc_vs].iov_base, (pos)))
#define pl_ptr(pos) (T_ptr(B->buffers[flatcc_builder_alloc_pl].iov_base, (pos)))
#define vd_ptr(pos) (T_ptr(B->buffers[flatcc_builder_alloc_vd].iov_base, (pos)))
#define vb_ptr(pos) (T_ptr(B->buffers[flatcc_builder_alloc_vb].iov_base, (pos)))
#define vs_offset(ptr) ((size_t)(ptr) - (size_t)B->buffers[flatcc_builder_alloc_vs].iov_base)
#define pl_offset(ptr) ((size_t)(ptr) - (size_t)B->buffers[flatcc_builder_alloc_pl].iov_base)

#define table_limit (FLATBUFFERS_VOFFSET_MAX - field_size + 1)
#define data_limit (FLATBUFFERS_UOFFSET_MAX - field_size + 1)

#define set_identifier(id) memcpy(B->identifier, id ? id : _pad, identifier_size)

/* This also returns true if no buffer has been started. */
#define is_top_buffer(B) (B->buffer_mark == 0)

/*
 * Tables use a stack represention better suited for quickly adding
 * fields to tables, but it must occasionally be refreshed following
 * reallocation or reentry from child frame.
 */
static inline void refresh_ds(flatcc_builder_t * B, size_t type_limit)
{
    iovec_t *buf = B->buffers + flatcc_builder_alloc_ds;

    B->ds = ds_ptr(B->ds_first);
    B->ds_limit = buf->iov_len - B->ds_first;
    /*
     * So we don't allocate outside tables representation size, nor our
     * current buffer size.
     */
    if (B->ds_limit > type_limit) {
        B->ds_limit = type_limit;
    }
    /* So exit frame can refresh fast. */
    frame(type_limit) = type_limit;
}

static int reserve_ds(flatcc_builder_t *B, size_t need, size_t limit)
{
    iovec_t *buf = B->buffers + flatcc_builder_alloc_ds;

    if (B->alloc(B->alloc_context, buf, B->ds_first + need, 1, flatcc_builder_alloc_ds)) {
        return -1;
    }
    refresh_ds(B, limit);
    return 0;
}

/*
 * Make sure there is always an extra zero termination on stack
 * even if it isn't emitted such that string updates may count
 * on zero termination being present always.
 */
static inline void *push_ds(flatcc_builder_t *B, size_t size)
{
    size_t offset;

    offset = B->ds_offset;
    if ((B->ds_offset += size) >= B->ds_limit) {
        if (reserve_ds(B, B->ds_offset + 1, data_limit)) {
            return 0;
        }
    }
    return B->ds + offset;
}

static inline void unpush_ds(flatcc_builder_t *B, size_t size)
{
    assert(B->ds_offset > size);
    B->ds_offset -= size;
    memset(B->ds + B->ds_offset, 0, size);
}

static inline void *push_ds_copy(flatcc_builder_t *B, const void *data, size_t size)
{
    void *p;

    if (!(p = push_ds(B, size))) {
        return 0;
    }
    memcpy(p, data, size);
    return p;
}

static inline void *push_ds_field(flatcc_builder_t *B, size_t size, size_t align, voffset_t id)
{
    size_t offset;

    assert(B->vs[id] == 0 && "field already set");
    /*
     * We calculate table field alignment relative to first entry, not
     * header field with vtable offset.
     *
     * Note: >= comparison handles special case where B->ds is not
     * allocated yet and size is 0 so the return value would be mistaken
     * for an error.
     */
    offset = alignup(B->ds_offset, align);
    if ((B->ds_offset = offset + size) >= B->ds_limit) {
        if (reserve_ds(B, B->ds_offset + 1, table_limit)) {
            return 0;
        }
    }
    B->vs[id] = (voffset_t)(offset + field_size);
    if (id >= B->id_end) {
        B->id_end = id + 1;
    }
    return B->ds + offset;
}

static inline void *push_ds_offset_field(flatcc_builder_t *B, voffset_t id)
{
    size_t offset;

    assert(B->vs[id] == 0 && "field already set");
    offset = alignup(B->ds_offset, field_size);
    if ((B->ds_offset = offset + field_size) > B->ds_limit) {
        if (reserve_ds(B, B->ds_offset, table_limit)) {
            return 0;
        }
    }
    B->vs[id] = (voffset_t)(offset + field_size);
    if (id >= B->id_end) {
        B->id_end = id + 1;
    }
    *B->pl++ = offset;
    return B->ds + offset;
}

static inline void *reserve_buffer(flatcc_builder_t *B, int alloc_type, size_t used, size_t need, int zero_init)
{
    iovec_t *buf = B->buffers + alloc_type;

    if (used + need > buf->iov_len) {
        if (B->alloc(B->alloc_context, buf, used + need, zero_init, alloc_type)) {
            return 0;
        }
    }
    return (void *)((size_t)buf->iov_base + used);
}

static inline int reserve_fields(flatcc_builder_t *B, int count)
{
    uoffset_t used, need;

    /* Provide faster stack operations for common table operations. */
    used = frame(table.vs_end) + frame(table.id_end) * sizeof(voffset_t);
    need = (count + 2) * sizeof(voffset_t);
    if (!(B->vs = reserve_buffer(B, flatcc_builder_alloc_vs, used, need, 1))) {
        return -1;
    }
    /* Move past header for convenience. */
    B->vs += 2;
    used = frame(table.pl_end);
    need = count * sizeof(*(B->pl));
    if (!(B->pl = reserve_buffer(B, flatcc_builder_alloc_pl, used, need, 0))) {
        return -1;
    }
    return 0;
}

static int alloc_ht(flatcc_builder_t *B)
{
    iovec_t *buf = B->buffers + flatcc_builder_alloc_ht;

    size_t size;
    /* Allocate null entry so we can check for return errors. */
    assert(B->vd_end == 0);
    if (!reserve_buffer(B, flatcc_builder_alloc_vd, B->vd_end, sizeof(vtable_descriptor_t), 0)) {
        return -1;
    }
    B->vd_end = sizeof(vtable_descriptor_t);
    size = field_size * FLATCC_BUILDER_MIN_HASH_COUNT;
    if (B->alloc(B->alloc_context, buf, size, 1, flatcc_builder_alloc_ht)) {
        return -1;
    }
    while (size * 2 <= buf->iov_len) {
        size *= 2;
    }
    B->ht_mask = size / field_size - 1;
    return 0;
}

static inline uoffset_t *lookup_ht(flatcc_builder_t *B, uint32_t hash)
{
    uoffset_t *T;

    if (B->ht_mask == 0) {
        if (alloc_ht(B)) {
            return 0;
        }
    }
    T = B->buffers[flatcc_builder_alloc_ht].iov_base;
    return &T[hash & B->ht_mask];
}

void flatcc_builder_flush_vtable_cache(flatcc_builder_t *B)
{
    iovec_t *buf = B->buffers + flatcc_builder_alloc_ht;

    if (B->ht_mask == 0) {
        return;
    }
    memset(buf->iov_base, 0, buf->iov_len);
    /* Reserve the null entry. */
    B->vd_end = sizeof(vtable_descriptor_t);
    B->vb_end = 0;
}

int flatcc_builder_custom_init(flatcc_builder_t *B,
        flatcc_builder_emit_fun *emit, void *emit_context,
        flatcc_builder_alloc_fun *alloc, void *alloc_context)
{
    /*
     * Do not allocate anything here. Only the required buffers will be
     * allocated. For simple struct buffers, no allocation is required
     * at all.
     */
    memset(B, 0, sizeof(*B));

    if (emit == 0) {
        B->is_default_emitter = 1;
        emit = flatcc_emitter;
        emit_context = &B->default_emit_context;
    }
    if (alloc == 0) {
        alloc = flatcc_builder_default_alloc;
    }
    B->alloc_context = alloc_context;
    B->alloc = alloc;
    B->emit_context = emit_context;
    B->emit = emit;
    return 0;
}

int flatcc_builder_init(flatcc_builder_t *B)
{
    return flatcc_builder_custom_init(B, 0, 0, 0, 0);
}

int flatcc_builder_custom_reset(flatcc_builder_t *B, int set_defaults, int reduce_buffers)
{
    iovec_t *buf;
    int i;

    for (i = 0; i < FLATCC_BUILDER_ALLOC_BUFFER_COUNT; ++i) {
        buf = B->buffers + i;
        if (buf->iov_base) {
            /* Don't try to reduce the hash table. */
            if (i != flatcc_builder_alloc_ht &&
                reduce_buffers && B->alloc(B->alloc_context, buf, 1, 1, i)) {
                return -1;
            }
            memset(buf->iov_base, 0, buf->iov_len);
        } else {
            assert(buf->iov_len == 0);
        }
    }
    B->vb_end = 0;
    if (B->vd_end > 0) {
        /* Reset past null entry. */
        B->vd_end = sizeof(vtable_descriptor_t);
    }
    B->min_align = 0;
    B->emit_start = 0;
    B->emit_end = 0;
    B->level = 0;
    B->limit_level = 0;
    B->ds_offset = 0;
    B->ds_limit = 0;
    /* Needed for correct offset calculation. */
    B->ds = B->buffers[flatcc_builder_alloc_ds].iov_base;
    B->pl = B->buffers[flatcc_builder_alloc_pl].iov_base;
    B->vs = B->buffers[flatcc_builder_alloc_vs].iov_base;
    B->frame = 0;
    if (set_defaults) {
        B->vb_flush_limit = 0;
        B->max_level = 0;
        B->disable_vt_clustering = 0;
    }
    if (B->is_default_emitter) {
        flatcc_emitter_reset(&B->default_emit_context);
    }
    return 0;
}

int flatcc_builder_reset(flatcc_builder_t *B)
{
    return flatcc_builder_custom_reset(B, 0, 0);
}

void flatcc_builder_clear(flatcc_builder_t *B)
{
    iovec_t *buf;
    int i;

    for (i = 0; i < FLATCC_BUILDER_ALLOC_BUFFER_COUNT; ++i) {
        buf = B->buffers + i;
        B->alloc(B->alloc_context, buf, 0, 0, i);
    }
    if (B->is_default_emitter) {
        flatcc_emitter_clear(&B->default_emit_context);
    }
    memset(B, 0, sizeof(*B));
}

static inline void set_min_align(flatcc_builder_t *B, uint16_t align)
{
    if (B->min_align < align) {
        B->min_align = align;
    }
}

/*
 * It's a max, but the minimum viable alignment is the largest observed
 * alignment requirement, but no larger.
 */
static inline void get_min_align(uint16_t *align, uint16_t b)
{
    if (*align < b) {
        *align = b;
    }
}

static int enter_frame(flatcc_builder_t *B, uint16_t align)
{
    if (++B->level > B->limit_level) {
        if (B->max_level > 0 && B->level > B->max_level) {
            return -1;
        }
        if (!(B->frame = reserve_buffer(B, flatcc_builder_alloc_fs, B->level * frame_size, frame_size, 0))) {
            return -1;
        }
        B->limit_level = B->buffers[flatcc_builder_alloc_fs].iov_len / frame_size;
        if (B->max_level > 0 && B->max_level < B->limit_level) {
            B->limit_level = B->max_level;
        }
    } else {
        ++B->frame;
    }
    frame(ds_offset) = B->ds_offset;
    frame(user_state) = B->user_state;
    frame(align) = B->align;
    B->align = align;
    /* Note: do not assume padding before first has been allocated! */
    frame(ds_first) = B->ds_first;
    frame(type_limit) = data_limit;
    B->ds_first = alignup(B->ds_first + B->ds_offset, 8);
    B->ds_offset = 0;
    return 0;
}

static inline void exit_frame(flatcc_builder_t *B)
{
    memset(B->ds, 0, B->ds_offset);
    B->user_state = frame(user_state);
    B->ds_offset = frame(ds_offset);
    B->ds_first = frame(ds_first);
    refresh_ds(B, frame(type_limit));

    /*
     * Restore local alignment: e.g. a table should not change alignment
     * because a child table was just created elsewhere in the buffer,
     * but the overall alignment (min align), should be aware of it.
     * Each buffer has its own min align that then migrates up without
     * being affected by sibling or child buffers.
     */
    set_min_align(B, B->align);
    B->align = frame(align);

    --B->frame;
    --B->level;
}

static inline size_t front_pad(flatcc_builder_t *B, size_t size, uint16_t align)
{
    return ((size_t)B->emit_start - size) & (align - 1);
}

static inline size_t back_pad(flatcc_builder_t *B, uint16_t align)
{
    return ((size_t)B->emit_end) & (align - 1);
}

static inline flatcc_builder_ref_t emit_front(flatcc_builder_t *B, iov_state_t *iov)
{
    flatcc_builder_ref_t ref;

    /*
     * We might have overflow when including headers, but without
     * headers we should have checks to prevent overflow in the
     * uoffset_t range, hence we subtract 16 to be safe. With that
     * guarantee we can also make a safe check on the soffset_t range.
     *
     * We only allow buffers half the theoritical size of
     * FLATBUFFERS_UOFFSET_MAX so we can safely use signed references.
     *
     * NOTE: vtables vt_offset field is signed, and the check in create
     * table only ensures the signed limit. The check would fail if the
     * total buffer size could grow beyond UOFFSET_MAX, and we prevent
     * that by limiting the lower end to SOFFSET_MIN, and the upper end
     * at emit_back to SOFFSET_MAX.
     */
    ref = B->emit_start - (flatcc_builder_ref_t)iov->len;
    if ((iov->len > 16 && iov->len - 16 > FLATBUFFERS_UOFFSET_MAX) || ref >= B->emit_start) {
        return 0;
    }
    if (B->emit(B->emit_context, iov->iov, iov->count, ref, iov->len)) {
        return 0;
    }
    return B->emit_start = ref;
}

static inline flatcc_builder_ref_t emit_back(flatcc_builder_t *B, iov_state_t *iov)
{
    flatcc_builder_ref_t ref;

    ref = B->emit_end;
    B->emit_end = ref + (flatcc_builder_ref_t)iov->len;
    /*
     * Similar to emit_front check, but since we only emit vtables and
     * padding at the back, we are not concerned with iov->len
     * overflow, only total buffer overflow.
     *
     * With this check, vtable soffset references at table header can
     * still overflow in extreme cases, so this must be checked
     * separately.
     */
    if (B->emit_end < ref) {
        return 0;
    }
    if (B->emit(B->emit_context, iov->iov, iov->count, ref, iov->len)) {
        return 0;
    }
    /*
     * Back references always return ref + 1 because ref == 0 is valid and
     * should not be mistaken for error. vtables understand this.
     */
    return ref + 1;
}

static int align_to_block(flatcc_builder_t *B, uint16_t *align, uint16_t block_align, int is_nested)
{
    size_t end_pad;
    iov_state_t iov;

    block_align = block_align ? block_align : B->block_align ? B->block_align : 1;
    get_min_align(align, field_size);
    get_min_align(align, block_align);
    /* Pad end of buffer to multiple. */
    if (!is_nested) {
        end_pad = back_pad(B, block_align);
        if (end_pad) {
            init_iov();
            push_iov(_pad, end_pad);
            if (0 == emit_back(B, &iov)) {
                return -1;
            }
        }
    }
    return 0;
}

flatcc_builder_ref_t flatcc_builder_embed_buffer(flatcc_builder_t *B,
        uint16_t block_align,
        const void *data, size_t size, uint16_t align)
{
    size_t size_field, pad;
    iov_state_t iov;

    if (align_to_block(B, &align, block_align, !is_top_buffer(B))) {
        return 0;
    }
    pad = front_pad(B, size, align);
    size_field = size + pad;
    size_field = store_uoffset(size_field);
    init_iov();
    /* Add ubyte vector size header if nested buffer. */
    push_iov_cond(&size_field, field_size, !is_top_buffer(B));
    push_iov(data, size);
    push_iov(_pad, pad)
    return emit_front(B, &iov);
}

flatcc_builder_ref_t flatcc_builder_create_buffer(flatcc_builder_t *B,
         const char identifier[identifier_size], uint16_t block_align,
         flatcc_builder_ref_t object_ref, uint16_t align, int is_nested)
{
    flatcc_builder_ref_t buffer_ref;
    size_t header_pad, id_size;
    uoffset_t object_offset, buffer_size, buffer_base;
    iov_state_t iov;

    if (align_to_block(B, &align, block_align, is_nested)) {
        return 0;
    }
    set_min_align(B, align);
    id_size = identifier_size;
    /* Identifiers are not always present in buffer. */
    if (!identifier || 0 == memcmp(identifier, _pad, identifier_size)) {
        id_size = 0;
    }
    header_pad = front_pad(B, field_size + id_size, align);
    init_iov();
    /* ubyte vectors size field wrapping nested buffer. */
    push_iov_cond(&buffer_size, field_size, is_nested);
    push_iov(&object_offset, field_size);
    push_iov(identifier, id_size);
    push_iov(_pad, header_pad);
    buffer_base = (uoffset_t)B->emit_start - (uoffset_t)iov.len + (is_nested ? field_size : 0);
    buffer_size = (uoffset_t)B->buffer_mark - buffer_base;
    buffer_size = store_uoffset(buffer_size);
    object_offset = (uoffset_t)object_ref - buffer_base;
    object_offset = store_uoffset(object_offset);
    if (0 == (buffer_ref = emit_front(B, &iov))) {
        return 0;
    }
    return buffer_ref;
}

flatcc_builder_ref_t flatcc_builder_create_struct(flatcc_builder_t *B, const void *data, size_t size, uint16_t align)
{
    size_t pad;
    iov_state_t iov;

    assert(align >= 1);
    set_min_align(B, align);
    pad = front_pad(B, size, align);
    init_iov();
    push_iov(data, size);
    /*
     * Normally structs will already be a multiple of their alignment,
     * so this padding will not likely be emitted.
     */
    push_iov(_pad, pad);
    return emit_front(B, &iov);
}

int flatcc_builder_start_buffer(flatcc_builder_t *B,
        const char identifier[identifier_size], uint16_t block_align)
{
    /*
     * This saves the parent `min_align` in the align field since we
     * shouldn't use that for the current buffer. `exit_frame`
     * automatically aggregates align up, so it is updated when the
     * buffer frame exits.
     */
    if (enter_frame(B, B->min_align)) {
        return -1;
    }
    /* B->align now has parent min_align, and child frames will save it. */
    B->min_align = 1;
    /* Save the parent block align, and set proper defaults for this buffer. */
    frame(buffer.block_align) = B->block_align;
    B->block_align = block_align;
    frame(buffer.mark) = B->buffer_mark;
    /* Allow vectors etc. to be constructed before buffer at root level. */
    B->buffer_mark = B->level == 1 ? 0 : B->emit_start;
    memcpy(frame(buffer.identifier), B->identifier, identifier_size);
    memcpy(B->identifier, identifier ? identifier : (const char *)_pad, identifier_size);
    frame(type) = flatcc_builder_buffer;
    return 0;
}

flatcc_builder_ref_t flatcc_builder_end_buffer(flatcc_builder_t *B, flatcc_builder_ref_t root)
{
    flatcc_builder_ref_t buffer_ref;

    assert(frame(type) == flatcc_builder_buffer);
    set_min_align(B, B->block_align);
    if (0 == (buffer_ref = flatcc_builder_create_buffer(B, B->identifier,
                    B->block_align, root, B->min_align, !is_top_buffer(B)))) {
        return 0;
    }
    B->buffer_mark = frame(buffer.mark);
    memcpy(B->identifier, frame(buffer.identifier), identifier_size);
    exit_frame(B);
    return buffer_ref;
}

void *flatcc_builder_start_struct(flatcc_builder_t *B, size_t size, uint16_t align)
{
    /* Allocate space for the struct on the ds stack. */
    if (enter_frame(B, align)) {
        return 0;
    }
    frame(type) = flatcc_builder_struct;
    return push_ds(B, size);
}

void *flatcc_builder_struct_edit(flatcc_builder_t *B)
{
    return B->ds;
}

flatcc_builder_ref_t flatcc_builder_end_struct(flatcc_builder_t *B)
{
    flatcc_builder_ref_t object_ref;

    assert(frame(type) == flatcc_builder_struct);
    if (0 == (object_ref = flatcc_builder_create_struct(B, B->ds, B->ds_offset, B->align))) {
        return 0;
    }
    exit_frame(B);
    return object_ref;
}

static inline int vector_count_add(flatcc_builder_t *B, size_t count, size_t max_count)
{
    size_t n, n1;
    n = frame(vector.count);
    n1 = n + count;
    /*
     * This prevents elem_size * count from overflowing iff max_vector
     * has been set sensible. Without this check we might allocate to
     * little on the ds stack and return a buffer the user thinks is
     * much larger which of course is bad even though the buffer eventually
     * would fail anyway.
     */
    if (n1 < n || n1 > max_count) {
        return -1;
    }
    frame(vector.count) = n1;
    return 0;
}

void *flatcc_builder_extend_vector(flatcc_builder_t *B, size_t count)
{
    if (vector_count_add(B, count, frame(vector.max_count))) {
        return 0;
    }
    return push_ds(B, frame(vector.elem_size) * count);
}

void *flatcc_builder_vector_push(flatcc_builder_t *B, const void *data)
{
    assert(frame(type) == flatcc_builder_vector);
    if (frame(vector.count) >= frame(vector.max_count)) {
        return 0;
    }
    frame(vector.count) += 1;
    return push_ds_copy(B, data, frame(vector.elem_size));
}

void *flatcc_builder_append_vector(flatcc_builder_t *B, const void *data, size_t count)
{
    assert(frame(type) == flatcc_builder_vector);
    if (vector_count_add(B, count, frame(vector.max_count))) {
        return 0;
    }
    return push_ds_copy(B, data, frame(vector.elem_size) * count);
}

flatcc_builder_ref_t *flatcc_builder_extend_offset_vector(flatcc_builder_t *B, size_t count)
{
    if (vector_count_add(B, count, max_offset_count)) {
        return 0;
    }
    return push_ds(B, field_size * count);
}

flatcc_builder_ref_t *flatcc_builder_offset_vector_push(flatcc_builder_t *B, flatcc_builder_ref_t ref)
{
    flatcc_builder_ref_t *p;

    assert(frame(type) == flatcc_builder_offset_vector);
    if (frame(vector.count) == max_offset_count) {
        return 0;
    }
    frame(vector.count) += 1;
    if (0 == (p = push_ds(B, field_size))) {
        return 0;
    }
    *p = ref;
    return p;
}

flatcc_builder_ref_t *flatcc_builder_append_offset_vector(flatcc_builder_t *B, const flatcc_builder_ref_t *refs, size_t count)
{
    assert(frame(type) == flatcc_builder_offset_vector);
    if (vector_count_add(B, count, max_offset_count)) {
        return 0;
    }
    return push_ds_copy(B, refs, field_size * count);
}

char *flatcc_builder_extend_string(flatcc_builder_t *B, size_t len)
{
    assert(frame(type) == flatcc_builder_string);
    if (vector_count_add(B, len, max_string_len)) {
        return 0;
    }
    return push_ds(B, len);
}

char *flatcc_builder_append_string(flatcc_builder_t *B, const char *s, size_t len)
{
    assert(frame(type) == flatcc_builder_string);
    if (vector_count_add(B, len, max_string_len)) {
        return 0;
    }
    return push_ds_copy(B, s, len);
}

char *flatcc_builder_append_string_str(flatcc_builder_t *B, const char *s)
{
    return flatcc_builder_append_string(B, s, strlen(s));
}

char *flatcc_builder_append_string_strn(flatcc_builder_t *B, const char *s, size_t max_len)
{
    return flatcc_builder_append_string(B, s, strnlen(s, max_len));
}

int flatcc_builder_truncate_vector(flatcc_builder_t *B, size_t count)
{
    assert(frame(type) == flatcc_builder_vector);
    assert(frame(vector.count) >= count);
    if (frame(vector.count) < count) {
        return -1;
    }
    frame(vector.count) -= count;
    unpush_ds(B, frame(vector.elem_size) * count);
    return 0;
}

int flatcc_builder_truncate_offset_vector(flatcc_builder_t *B, size_t count)
{
    assert(frame(type) == flatcc_builder_offset_vector);
    assert(frame(vector.count) >= count);
    if (frame(vector.count) < count) {
        return -1;
    }
    frame(vector.count) -= count;
    unpush_ds(B, frame(vector.elem_size) * count);
    return 0;
}

int flatcc_builder_truncate_string(flatcc_builder_t *B, size_t len)
{
    assert(frame(type) == flatcc_builder_string);
    assert(frame(vector.count) >= len);
    if (frame(vector.count) < len) {
        return -1;
    }
    frame(vector.count) -= len;
    unpush_ds(B, len);
    return 0;
}

void *flatcc_builder_start_vector(flatcc_builder_t *B, size_t elem_size, uint16_t align, size_t count, size_t max_count)
{
    get_min_align(&align, field_size);
    if (enter_frame(B, align)) {
        return 0;
    }
    frame(vector.elem_size) = elem_size;
    frame(vector.count) = 0;
    frame(vector.max_count) = max_count;
    frame(type) = flatcc_builder_vector;
    refresh_ds(B, data_limit);

    return flatcc_builder_extend_vector(B, count);
}

flatcc_builder_ref_t *flatcc_builder_start_offset_vector(flatcc_builder_t *B, size_t count)
{
    if (enter_frame(B, field_size)) {
        return 0;
    }
    frame(vector.elem_size) = field_size;
    frame(vector.count) = 0;
    frame(type) = flatcc_builder_offset_vector;
    refresh_ds(B, data_limit);
    return flatcc_builder_extend_offset_vector(B, count);
}

flatcc_builder_ref_t flatcc_builder_create_offset_vector(flatcc_builder_t *B,
        const flatcc_builder_ref_t *vec, size_t count)
{
    flatcc_builder_ref_t *_vec;

    if (!(_vec = flatcc_builder_start_offset_vector(B, count))) {
        return 0;
    }
    memcpy(_vec, vec, count * field_size);
    return flatcc_builder_end_offset_vector(B);
}

char *flatcc_builder_start_string(flatcc_builder_t *B, size_t len)
{
    if (enter_frame(B, 1)) {
        return 0;
    }
    frame(vector.elem_size) = 1;
    frame(vector.count) = 0;
    frame(type) = flatcc_builder_string;
    refresh_ds(B, data_limit);
    return flatcc_builder_extend_string(B, len);
}

int flatcc_builder_reserve_table(flatcc_builder_t *B, int count)
{
    assert(count >= 0);
    return reserve_fields(B, count);
}

int flatcc_builder_start_table(flatcc_builder_t *B, int count)
{
    if (enter_frame(B, field_size)) {
        return -1;
    }
    frame(table.vs_end) = vs_offset(B->vs);
    frame(table.pl_end) = pl_offset(B->pl);
    frame(table.vt_hash) = B->vt_hash;
    frame(table.id_end) = B->id_end;
    B->vt_hash = 0;
    FLATCC_BUILDER_INIT_VT_HASH(B->vt_hash);
    B->id_end = 0;
    frame(type) = flatcc_builder_table;
    if (reserve_fields(B, count)) {
        return -1;
    }
    refresh_ds(B, table_limit);
    return 0;
}

flatcc_builder_vt_ref_t flatcc_builder_create_vtable(flatcc_builder_t *B,
        const voffset_t *vt, voffset_t vt_size)
{
    flatcc_builder_vt_ref_t vt_ref;
    iov_state_t iov;

    /*
     * Only top-level buffer can cluster vtables because only it can
     * extend beyond the end.
     *
     * We write the vtable after the referencing table to maintain
     * the construction invariant that any offset reference has
     * valid emitted data at a higher address, and also that any
     * issued negative emit address represents an offset reference
     * to some flatbuffer object or vector (or possibly a root
     * struct).
     *
     * The vt_ref is stored as the reference + 1 to avoid having 0 as a
     * valid reference (which usally means error). It also idententifies
     * vtable references as the only uneven references, and the only
     * references that can be used multiple times in the same buffer.
     */
    init_iov();
    push_iov(vt, vt_size);
    if (is_top_buffer(B) && !B->disable_vt_clustering) {
        /* Note that `emit_back` already returns ref + 1 as we require for vtables. */
        if (0 == (vt_ref = emit_back(B, &iov))) {
            return 0;
        }
    } else {
        if (0 == (vt_ref = emit_front(B, &iov))) {
            return 0;
        }
        /*
         * We don't have a valid 0 ref here, but to be consistent with
         * clustered vtables we offset by one. This cannot be zero
         * either.
         */
        vt_ref += 1;
    }
    return vt_ref;
}

flatcc_builder_vt_ref_t flatcc_builder_create_cached_vtable(flatcc_builder_t *B,
        const voffset_t *vt, voffset_t vt_size, uint32_t vt_hash)
{
    vtable_descriptor_t *vd, *vd2;
    uoffset_t *pvd, *pvd_head;
    uoffset_t next;
    voffset_t *vt_;
    voffset_t encoded_vt_size;

    /* This just gets the hash table slot, we still have to inspect it. */
    if (!(pvd_head = lookup_ht(B, vt_hash))) {
        return 0;
    }
    pvd = pvd_head;
    next = *pvd;
    /* Tracks if there already is a cached copy. */
    vd2 = 0;
    encoded_vt_size = vt[0];
    while (next) {
        vd = vd_ptr(next);
        vt_ = vb_ptr(vd->vb_start);
        if (vt_[0] != encoded_vt_size || 0 != memcmp(vt, vt_, vt_size)) {
            pvd = &vd->next;
            next = vd->next;
            continue;
        }
        /* Can't share emitted vtables between buffers, */
        if (vd->buffer_mark != B->buffer_mark) {
            /* but we don't have to resubmit to cache. */
            vd2 = vd;
            /* See if there is a better match. */
            pvd = &vd->next;
            next = vd->next;
            continue;
        }
        /* Move to front hash strategy. */
        if (pvd != pvd_head) {
            *pvd = next;
            vd->next = *pvd_head;
            *pvd_head = next;
        }
        /* vtable exists and has been emitted within current buffer. */
        return vd->vt_ref;
    }
    /* Allocate new descriptor. */
    if (!(vd = reserve_buffer(B, flatcc_builder_alloc_vd, B->vd_end, sizeof(vtable_descriptor_t), 0))) {
        return 0;
    }
    next = B->vd_end;
    B->vd_end += sizeof(vtable_descriptor_t);

    /* Identify the buffer this vtable descriptor belongs to. */
    vd->buffer_mark = B->buffer_mark;

    /* Move to front hash strategy. */
    vd->next = *pvd_head;
    *pvd_head = next;
    if (0 == (vd->vt_ref = flatcc_builder_create_vtable(B, vt, vt_size))) {
        return 0;
    }
    if (vd2) {
        /* Reuse cached copy. */
        vd->vb_start = vd2->vb_start;
    } else {
        if (B->vb_flush_limit && B->vb_flush_limit < B->vb_end + vt_size) {
            flatcc_builder_flush_vtable_cache(B);
        } else {
            /* Make space in vtable cache. */
            if (!(vt_ = reserve_buffer(B, flatcc_builder_alloc_vb, B->vb_end, vt_size, 0))) {
                return -1;
            }
            vd->vb_start = B->vb_end;
            B->vb_end += vt_size;
            memcpy(vt_, vt, vt_size);
        }
    }
    return vd->vt_ref;
}

flatcc_builder_ref_t flatcc_builder_create_table(flatcc_builder_t *B, const void *data, size_t size, uint16_t align,
        flatbuffers_voffset_t *offsets, int offset_count, flatcc_builder_vt_ref_t vt_ref)
{
    int i;
    size_t pad;
    uoffset_t vt_offset, vt_base, base, offset, *offset_field;
    iov_state_t iov;

    assert(offset_count >= 0);
    /*
     * vtable references are offset by 1 to avoid confusion with
     * 0 as an error reference. It also uniquely identifies them
     * as vtables being the only uneven reference type.
     */
    assert(vt_ref & 1);
    get_min_align(&align, field_size);
    set_min_align(B, align);
    /* Alignment is calculated for the first element, not the header. */
    pad = front_pad(B, size, align);
    base = (uoffset_t)B->emit_start - (uoffset_t)(pad + size + field_size);
    /* Adjust by 1 to get unencoded vtable reference. */
    vt_base = (uoffset_t)(vt_ref - 1);
    vt_offset = base - vt_base;
    /* Avoid overflow. */
    if (base - vt_offset != vt_base) {
        return -1;
    }
    /* Protocol endian encoding. */
    vt_offset = store_uoffset(vt_offset);
    for (i = 0; i < offset_count; ++i) {
        offset_field = (uoffset_t *)((size_t)data + offsets[i]);
        offset = *offset_field - base - offsets[i] - field_size;
        offset = store_uoffset(offset);
        *offset_field = offset;
    }
    init_iov();
    push_iov(&vt_offset, field_size);
    push_iov(data, size);
    push_iov(_pad, pad);
    return emit_front(B, &iov);
}

int flatcc_builder_check_required(flatcc_builder_t *B, const flatbuffers_voffset_t *required, int count)
{
    int i;

    if (B->id_end < count) {
        return 0;
    }
    for (i = 0; i < count; ++i) {
        if (B->vs[required[i]] == 0) {
            return 0;
        }
    }
    return 1;
}

flatcc_builder_ref_t flatcc_builder_end_table(flatcc_builder_t *B)
{
    voffset_t *vt, vt_size;
    flatcc_builder_ref_t table_ref, vt_ref;
    int pl_count;
    voffset_t *pl;

    assert(frame(type) == flatcc_builder_table);

    /* We have `ds_limit`, so we should not have to check for overflow here. */

    vt = B->vs - 2;
    vt_size = sizeof(voffset_t) * (B->id_end + 2);
    /* Update vtable header fields, first vtable size, then object table size. */
    vt[0] = store_voffset(vt_size);
    /*
     * The `ds` buffer is always at least `field_size` aligned but excludes the
     * initial vtable offset field. Therefore `field_size` is added here
     * to the total table size in the vtable.
     */
    vt[1] = store_voffset((voffset_t)B->ds_offset + field_size);
    FLATCC_BUILDER_UPDATE_VT_HASH(B->vt_hash, (uint32_t)vt[0], (uint32_t)vt[1]);
    /* Find already emitted vtable, or emit a new one. */
    if (!(vt_ref = flatcc_builder_create_cached_vtable(B, vt, vt_size, B->vt_hash))) {
        return 0;
    }
    /* Clear vs stack so it is ready for the next vtable (ds stack is cleared by exit frame). */
    memset(vt, 0, vt_size);

    pl = pl_ptr(frame(table.pl_end));
    pl_count = (int)(B->pl - pl);
    if (0 == (table_ref = flatcc_builder_create_table(B, B->ds, B->ds_offset, B->align, pl, pl_count, vt_ref))) {
        return 0;
    }
    B->vt_hash = frame(table.vt_hash);
    B->id_end = frame(table.id_end);
    B->vs = vs_ptr(frame(table.vs_end));
    B->pl = pl_ptr(frame(table.pl_end));
    exit_frame(B);
    return table_ref;
}

flatcc_builder_ref_t flatcc_builder_create_vector(flatcc_builder_t *B,
        const void *data, size_t count, size_t elem_size, uint16_t align, size_t max_count)
{
    size_t vec_size, vec_pad;
    uoffset_t length_prefix;
    iov_state_t iov;

    if (count > max_count) {
        return 0;
    }
    get_min_align(&align, field_size);
    set_min_align(B, align);
    vec_size = count * elem_size;
    length_prefix = (uoffset_t)count;
    length_prefix = store_uoffset(length_prefix);
    /* Alignment is calculated for the first element, not the header. */
    vec_pad = front_pad(B, vec_size, align);
    init_iov();
    push_iov(&length_prefix, field_size);
    push_iov(data, vec_size);
    push_iov(_pad, vec_pad);
    return emit_front(B, &iov);
}

/*
 * Note: FlatBuffers official documentation states that the size field of a
 * vector is a 32-bit element count. It is not quite clear if the
 * intention is to have the size field be of type uoffset_t since tables
 * also have a uoffset_t sized header, or if the vector size should
 * remain unchanged if uoffset is changed to 16- or 64-bits
 * respectively. Since it makes most sense to have a vector compatible
 * with the addressable space, we choose to use uoffset_t as size field,
 * which remains compatible with the default 32-bit version of uoffset_t.
 */
flatcc_builder_ref_t flatcc_builder_end_vector(flatcc_builder_t *B)
{
    flatcc_builder_ref_t vector_ref;

    assert(frame(type) == flatcc_builder_vector);

    if (0 == (vector_ref = flatcc_builder_create_vector(B, B->ds,
                    frame(vector.count), frame(vector.elem_size),
                    B->align, frame(vector.max_count)))) {
        return 0;
    }
    exit_frame(B);
    return vector_ref;
}

size_t flatcc_builder_vector_count(flatcc_builder_t *B)
{
    return frame(vector.count);
}

void *flatcc_builder_vector_edit(flatcc_builder_t *B)
{
    return B->ds;
}

/* This function destroys the source content but avoids stack allocation. */
flatcc_builder_ref_t flatcc_builder_create_offset_vector_direct(flatcc_builder_t *B,
        flatcc_builder_ref_t *vec, size_t count)
{
    size_t vec_size, vec_pad;
    uoffset_t length_prefix, base, offset;
    size_t i;
    iov_state_t iov;

    if (count > max_offset_count) {
        return 0;
    }
    set_min_align(B, field_size);
    vec_size = count * field_size;
    length_prefix = (uoffset_t)count;
    length_prefix = store_uoffset(length_prefix);
    /* Alignment is calculated for the first element, not the header. */
    vec_pad = front_pad(B, vec_size, field_size);
    init_iov();
    push_iov(&length_prefix, field_size);
    push_iov(vec, vec_size);
    push_iov(_pad, vec_pad);
    base = (uoffset_t)B->emit_start - (uoffset_t)iov.len;
    for (i = 0; i < count; ++i) {
        offset = vec[i] - base - i * field_size - field_size;
        vec[i] = store_uoffset(offset);
    }
    return emit_front(B, &iov);
}

flatcc_builder_ref_t flatcc_builder_end_offset_vector(flatcc_builder_t *B)
{
    flatcc_builder_ref_t vector_ref;

    assert(frame(type) == flatcc_builder_offset_vector);
    if (0 == (vector_ref = flatcc_builder_create_offset_vector_direct(B,
                    (flatcc_builder_ref_t *)B->ds, frame(vector.count)))) {
        return 0;
    }
    exit_frame(B);
    return vector_ref;
}

size_t flatcc_builder_offset_vector_count(flatcc_builder_t *B)
{
    return frame(vector.count);
}

void *flatcc_builder_offset_vector_edit(flatcc_builder_t *B)
{
    return B->ds;
}

flatcc_builder_ref_t flatcc_builder_create_string(flatcc_builder_t *B, const char *s, size_t len)
{
    size_t s_pad;
    uoffset_t length_prefix;
    iov_state_t iov;

    if (len > max_string_len) {
        return 0;
    }
    length_prefix = (uoffset_t)len;
    length_prefix = store_uoffset(length_prefix);
    /* Add 1 for zero termination. */
    s_pad = front_pad(B, len + 1, field_size) + 1;
    init_iov();
    push_iov(&len, field_size);
    push_iov(s, len);
    push_iov(_pad, s_pad);
    return emit_front(B, &iov);
}

flatcc_builder_ref_t flatcc_builder_create_string_str(flatcc_builder_t *B, const char *s)
{
    return flatcc_builder_create_string(B, s, strlen(s));
}

flatcc_builder_ref_t flatcc_builder_create_string_strn(flatcc_builder_t *B, const char *s, size_t max_len)
{
    return flatcc_builder_create_string(B, s, strnlen(s, max_len));
}


flatcc_builder_ref_t flatcc_builder_end_string(flatcc_builder_t *B)
{
    flatcc_builder_ref_t string_ref;

    assert(frame(type) == flatcc_builder_string);
    assert(frame(vector.count) == B->ds_offset);
    if (0 == (string_ref = flatcc_builder_create_string(B,
                    (const char *)B->ds, B->ds_offset))) {
        return 0;
    }
    exit_frame(B);
    return string_ref;
}

char *flatcc_builder_string_edit(flatcc_builder_t *B)
{
    return (char *)B->ds;
}

size_t flatcc_builder_string_len(flatcc_builder_t *B)
{
    return frame(vector.count);
}

void *flatcc_builder_table_add(flatcc_builder_t *B, int id, size_t size, uint16_t align)
{
    /*
     * We align the offset relative to the first table field, excluding
     * the header holding the vtable reference. On the stack, `ds_first`
     * is aligned to 8 bytes thanks to the `enter_frame` logic, and this
     * provides a safe way to update the fields on the stack, but here
     * we are concerned with the target buffer alignment.
     *
     * We could also have aligned relative to the end of the table which
     * would allow us to emit each field immediately, but it would be a
     * confusing user experience wrt. field ordering, and it would add
     * more variability to vtable layouts, thus reducing reuse, and
     * frequent emissions to external emitter interface would be
     * sub-optimal. Also, with that appoach, the vtable offsets would
     * have to be adjusted at table end.
     *
     * As we have it, each emit occur at table end, vector end, string
     * end, or buffer end, which might be helpful to various backend
     * processors.
     */
    assert(frame(type) == flatcc_builder_table);
    assert(id >= 0 && id <= (int)FLATBUFFERS_ID_MAX);
    if (align > B->align) {
        B->align = align;
    }
    FLATCC_BUILDER_UPDATE_VT_HASH(B->vt_hash, (uint32_t)id, (uint32_t)size);
    return push_ds_field(B, size, align, id);
}

void *flatcc_builder_table_edit(flatcc_builder_t *B, size_t size)
{
    return B->ds + B->ds_offset - size;
}

void *flatcc_builder_table_add_copy(flatcc_builder_t *B, int id, const void *data, size_t size, uint16_t align)
{
    void *p;

    if ((p = flatcc_builder_table_add(B, id, size, align))) {
        memcpy(p, data, size);
    }
    return p;
}

flatcc_builder_ref_t *flatcc_builder_table_add_offset(flatcc_builder_t *B, int id)
{
    assert(frame(type) == flatcc_builder_table);
    assert(id >= 0 && id <= (int)FLATBUFFERS_ID_MAX);
    FLATCC_BUILDER_UPDATE_VT_HASH(B->vt_hash, (uint32_t)id, (uint32_t)field_size);
    return push_ds_offset_field(B, id);
}

uint16_t flatcc_builder_push_buffer_alignment(flatcc_builder_t *B)
{
    uint16_t old_min_align = B->min_align;

    B->min_align = field_size;
    return old_min_align;
}

void flatcc_builder_pop_buffer_alignment(flatcc_builder_t *B, uint16_t pushed_align)
{
    set_min_align(B, pushed_align);
}

uint16_t flatcc_builder_get_buffer_alignment(flatcc_builder_t *B)
{
    return B->min_align;
}

void flatcc_builder_set_vtable_clustering(flatcc_builder_t *B, int enable)
{
    /* Inverted because we zero all memory in B on init. */
    B->disable_vt_clustering = !enable;
}

void flatcc_builder_set_block_align(flatcc_builder_t *B, uint16_t align)
{
    B->block_align = align;
}

int flatcc_builder_get_level(flatcc_builder_t *B)
{
    return B->level;
}

void flatcc_builder_set_max_level(flatcc_builder_t *B, int max_level)
{
    B->max_level = max_level;
    if (B->limit_level < B->max_level) {
        B->limit_level = B->max_level;
    }
}

size_t flatcc_builder_get_buffer_size(flatcc_builder_t *B)
{
    return (size_t)(B->emit_end - B->emit_start);
}

flatcc_builder_ref_t flatcc_builder_get_buffer_start(flatcc_builder_t *B)
{
    return B->emit_start;
}

flatcc_builder_ref_t flatcc_builder_get_buffer_end(flatcc_builder_t *B)
{
    return B->emit_end;
}

void flatcc_builder_set_vtable_cache_limit(flatcc_builder_t *B,
        size_t size)
{
    B->vb_flush_limit = size;
}

void flatcc_builder_set_identifier(flatcc_builder_t *B, const char identifier[identifier_size])
{
    memcpy(B->identifier, identifier ? identifier : (const char *)_pad, identifier_size);
}

void flatcc_builder_set_state(flatcc_builder_t *B,
        size_t state)
{
    B->user_state = state;
}

size_t flatcc_builder_get_state(flatcc_builder_t *B)
{
    return B->user_state;
}

size_t flatcc_builder_get_state_at(flatcc_builder_t *B, int level)
{
    if (level < 1 || level > B->level) {
        return 0;
    }
    return B->frame[level - B->level].user_state;
}

enum flatcc_builder_type flatcc_builder_get_type(flatcc_builder_t *B)
{
    return B->frame ? frame(type) : flatcc_builder_empty;
}

enum flatcc_builder_type flatcc_builder_get_type_at(flatcc_builder_t *B, int level)
{
    if (level < 1 || level > B->level) {
        return flatcc_builder_empty;
    }
    return B->frame[level - B->level].user_state;
}

void *flatcc_builder_get_direct_buffer(flatcc_builder_t *B, size_t *size_out)
{
    if (B->is_default_emitter) {
        return flatcc_emitter_get_direct_buffer(&B->default_emit_context, size_out);
    } else {
        if (size_out) {
            *size_out = 0;
        }
    }
    return 0;
}

void *flatcc_builder_copy_buffer(flatcc_builder_t *B, void *buffer, size_t size)
{
    if (!B->is_default_emitter) {
        return 0;
    }
    return flatcc_emitter_copy_buffer(&B->default_emit_context, buffer, size);
}

void *flatcc_builder_finalize_buffer(flatcc_builder_t *B, size_t *size_out)
{
    void * buffer;
    size_t size;

    size = flatcc_builder_get_buffer_size(B);

    if (size_out) {
        *size_out = size;
    }

    buffer = malloc(size);

    if (!buffer) {
        goto done;
    }
    if (!flatcc_builder_copy_buffer(B, buffer, size)) {
        free(buffer);
        buffer = 0;
    }
done:
    if (!buffer && size_out) {
        *size_out = 0;
    }
    return buffer;
}

void *flatcc_builder_finalize_aligned_buffer(flatcc_builder_t *B, size_t *size_out)
{
    void * buffer;
    size_t align;
    size_t size;

    size = flatcc_builder_get_buffer_size(B);

    if (size_out) {
        *size_out = size;
    }
    align = flatcc_builder_get_buffer_alignment(B);

#ifdef FLATBUFFERS_HAVE_ALIGNED_ALLOC
    size = (size + align - 1) & ~(align - 1);
    buffer = aligned_alloc(align, size);
#else
    buffer = (void *)(((size_t)malloc(size + align - 1) + align - 1) & ~(align - 1));
#endif

    if (!buffer) {
        goto done;
    }
    if (!flatcc_builder_copy_buffer(B, buffer, size)) {
        free(buffer);
        buffer = 0;
        goto done;
    }
done:
    if (!buffer && size_out) {
        *size_out = 0;
    }
    return buffer;
}

void *flatcc_builder_get_emit_context(flatcc_builder_t *B)
{
    return B->emit_context;
}
