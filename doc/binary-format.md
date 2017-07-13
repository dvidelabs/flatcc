# FlatBuffers Binary Format

## Overview

A FlatBuffers layout consists of the following types of memory blocks:

- header
- table
- vector
- string
- vtable

Each of these have a contigous memory layout. The header references the
root table. Every table references a vtable and stores field in a single
block of memory. Some of these fields can be offsets to vectors, strings
and vectors. Vectors are one-dimensional blocks where each element is
self-contained or stores reference to table or a string based on schema
type information. A vtable decides which fields are present in table and
where they are stored. Vtables are the key to support schema evolution.
A table has no special rules about field ordering except fields must be
properly aligned, and if they are ordered by size the will pack more
densely but possibly more complicated to construct and the schema may
provide guidelines on the preferred order. Two vtables might mean
different things but accidentally have the same structure. Such vtables
can be shared between different tables. vtable sharing is important when
vectors store many similar tables.

Space between the above blocks are zero padded and present in order to
ensure proper alignment. Structs must be packed as densely as possible
according the alignment rules that apply - this ensures that all
implements will agree on the layout. The blocks must not overlap in
memory but two blocks may be shared if they represent the same data such
as sharing a string.

FlatBuffers are constructed back to front such that lower level objects
such as sub-tables and strings are created first in stored last and such
that the root object is stored last and early in the buffer. See also
(Stream Buffers](#stream-buffers) for a proposed variation over this
theme.

All addressing in FlatBuffers are relative. The reason for this? When
navigating the buffer you don't need to store the buffer start or the
buffer length, you can also find a new reference relative the reference
you already have. vtables require the table location to find a field,
but a table field referencing a string field only needs the table field
location, not the table start in order to find the referenced string.
This results in efficient navigation.


## Example

Uoffsets add their content to their address and are positive while a
tables offset to its vtable is signed and is substracted. A vtables
element is added to the soffset location to originally pointed to the
vtable, i.e. the start of the referring table.


    Schema:

        enum Fruit : byte { Banana = -1, Orange = 42 }
        table FooBar {
            meal      : Fruit;
            attitude  : (deprecated);
            say       : String;
            height    : short;
        }
        file_identifer "NOOB";

    JSON:
        { "meal": "Orange", "say": "hello", "height": -8000 }

    Buffer:

        header:
            +0x0000 00 01 00 00 ; find root table at offset +0x0000100.
            +0x0000 'N', 'O', 'O', 'B' ; possibly a file identifier
           ... or perhaps a vtable, probably not padding since non-zero.

        root-table:
            +0x0100 e0 ff ff ff ; 32-bit soffset to vtable location
                                ; two's complement: 2^32 - 0xffffffe0 = -0x20
                                ; effective address: +0x0100 - (-0x20) = +0x0204
            +0x0104 00 01 00 00 ; 32-bit uoffset string field (FooBar.say)
                                ; find string +0x100 = 256 bytes _from_ here
                                ; = +0x0104 + 0x100 = +0x0204.
            +0x0108 42d         ; 8-bit (FooBar.meal)
            +0x0109 0           ; 8-bit padding
            +0x010a -8000d      ; 16-bit (FooBar.height)
            +0x010c 0, ...      ; (after table)
                    0, ...      ; lots of valid but unnecessary padding
                                ; to simplify math.
        vtable:
            +0x0120 08 00       ; vtable length = 8 bytes
            +0x0122 0c 00       ; table length = 12 bytes
            +0x0124 08 00       ; field id 0: +0x08 (meal)
            +0x0126 00 00       ; field id 1: <missing> (attitude)
            +0x0128 04 00       ; field id 2: +0004 (say)
            +0x012a 0a 00       ; field id 3: +0x0a (height) 

        string: 
            +0x0204 5 (vector element count)
            +0x0205 'h', 'e', 'l', 'l', 'o' (vector data)
            +0x020a '\0' (zero termination special case for string vectors)

Note that FlatCC often places vtables last resuting in `e0 ff ff ff`
style vtable offsets, while Googles flatc builder typically places them
before the table resulting in `20 00 00 00` style vtable offsets which
might help understand why the soffset is subtracted from and not added
to the table start. Both forms are equally valid.

## Internals

Please observe that data types have a standard size such as
`sizeof(uoffset_t) == 4` but the size might change for related formats.
Therefore the type name is important in the discussion.

All content is little endian and offsets are 4 bytes (`uoffset_t`).
A new buffer location is found by adding a uoffset to the location where
the offset is stored. The first location (offset 0) points to the root
table. `uoffset_t` based references are to tables, vectors, and strings.
References to vtables and to table fields within a table have a
different calculations as discussed below.

A late addition to the format allow for adding a size prefix before the
standard header. When this is done, the builder must know about so it
can align content according to the changed starting position. Receivers
must also know about the size field just as they must know about the
excpected buffer type.

The next 4 bytes (`sizeof(uoffset_t)`) might represent a 4 byte buffer
identifier, or it might be absent but there is no obvious way to know
which.  The file identifier is typically ASCII characters from the
schemas `file_identifier` field padded with 0 bytes but may also contain
any custom binary identifier in little endian encoding. See
[Type-Hashes](#type-hashes). The 0 identifier should be avoided because
the buffer might accidentally contain zero padding when an identifier is
absent and because 0 can be used by API's to speficy that no identifier
should be stored.

When reading a buffer, it should be checked that the length is at least
8 bytes (2 * `sizeof(uoffset_t)`) Otherwise it is not safe to check the
file identifier.

The root table starts with a 4 byte vtable offset (`soffset_t`). The
`soffset_t` has the same size as `uoffset_t` but is signed.

The vtable is found by *subtracting* the signed 4 byte offset to the
location where the vtable offset is stored. Note that the `FlatCC`
builder typically stores vtables at the end of the buffer (clustered
vtables) and therefore the vtable offset is normally negative. Other
builders often store the vtable before the table unless reusing an
existing vtable and this makes the soffset positive.
(Nested FlatBuffers will not store vtables at the end because it would
break compartmentalization).

The vtable is a table of 2 byte offsets (`voffset_t`). The first two
entreis are the vtable size in bytes and the table size in bytes. The
next offset is the vtable entry for table field 0. A vtable will always
have as many entries as the largest stored field in the table, but it
might skip entries that are not needed or known (version independence) -
therefore the vtable length must be checked. The table length is only
needed by verifiers. A vtable lookup of an out of range table id is the
same as a vtable entry that has a zero entry and results in a default
value for the expected type. Multiple tables may shared the same vtable,
even if they have different types. This is done via deduplication during
buffer construction. A table field id maps to a vtable offset using the
formula `vtable-start + sizeof(voffset_t) * (field-id + 2)`, iff the
result is within the vtable size.

The vtable entry stores a byte offset relative to the location where the
`soffset_t` where stored at the start of the table. A table field is
stored immediately after the `soffset_t` or may contain 0 padding. A
table field is always aligned to at least its own size, or more if the
schema demands it. Strings, sub-tables and vectors are stored as a
4-byte `uoffset_t`. Such sub-elements are always found later in the
buffer because `uoffset_t` is unsigned. A struct is stored in-place.
Conceptually a struct is not different from an integer in this respect,
just larger.

If a sub-table or vector is absent and not just without fields, 0
length, the containing table field pointing the sub-table or vector
should be absent. Note that structs cannot contain sub-tables or
vectors.

A struct is always 0 padded up to its alignment. A structs a alignment
is given as the largest size of any non-struct member, or the alignment
of a struct member (but not necessarily the size of a struct member), or
the schema specified aligment.

A structs members are stored in-place so there struct fields are known
via the the schema, not via information in the binary format

An enum is represent as its underlying integer type and can be a member
of structs, fields and vectors just like integer and floating point
scalars.

A table is always aligned to `sizeof(uoffset_t)`, but may contain
internal fields with larger alignment. That is, the table start or end
is not affected by alignment requirements of field members unlike
structs.

Strings are a special case of vectors, so the following also applies to
strings.

A vector starts with a `uoffset_t` field the gives the length in element
counts, not in bytes. Table fields points to the length field of a
vector, not the first element. A vector may have 0 length. Note that
the FlatCC C interface will return a pointer to a vectors first element
and not the vector length although the internal reference does not.

All elements of a vector has the same type which is either a scalar type
including enums, a struct, or a uoffset reference where all the
reference type is to tables all of the same type, all strings, or
references to tables of the same union for union vectors. It is not
possible to have vectors of vectors other than string vectors.

A vectors first element is aligned to the size of `uoffset_t` or the
alignment of its element type, or the alignment required by the schema,
whichever is larger. Note that the vector itself might be aligned to 4
bytes while the first element is aligned to 8 bytes.

(Currently the schema semantics do no support aligning vectors beyond
their element size, but that might change and can be forced when
building buffers via dedicated api calls).

Strings are stored as vectors of type `ubyte_t`, i.e. 8-bit elements. In
addition, a trailing zero is always stored. The trailing zero is not
counted in the length field of the vector. Technically a string is
supposed to be valid UTF-8, but in praxis it is permitted that strings
contain 0 bytes or other invalid sequences. It is recommended to
explicitly check strings for UTF-8 conformance when this is required
rather than to expect this to alwways be true. However, the ubyte vector
should be preferred for binary data.

A string, vector or table may be referenced by other tables and vectors.
This is known as a directed acyclic graph (DAG). Because uoffsets are
unsigned and because uoffsets are never stored with a zero value, it is
not possible for a buffer to contain cycles which makes them safe to
traverse with too much fear of excessive recursion. It also makes it
possible to efficiently verify that buffers do not point out of bounds.


## Type Hashes

A type hash is an almost unique binary identifier for every table type.
the identifier was not stored in the buffer or should not be stored in the
buffer. When storing a binary identifier it is recommended to use the
type hash convention which as the a 32-bit FNV-1a hash of the fully
qualified type name of the root table (`FNV-1a("MyGame.Example.Monster")`).
FlatCC generates these hashes automatically for every table and struct,
but they are easy to compute and yields predictable results across
implementations:


    static inline flatbuffers_thash_t flatbuffers_type_hash_from_name(const char *name)
    {
        uint32_t hash = 2166136261UL;
        while (*name) {
            hash ^= (uint32_t)*name;
            hash = hash * 16777619UL;
            ++name;
        }
        if (hash == 0) {
            hash = 2166136261UL;
        }
        return hash;
    }


## Unions

A union is a contruction on top of the above primitives. It consists of
a type and a value.

In the schema a union type is a set of table types with each table name
assigned a type enumaration starting from 1. 0 is the type NONE meaning
the union has not value assigned. The union type is represented as a
ubyte enum type, or in the binary format as a value of type
`union_type_t` which for standard FlatBuffers is an 8-bit unsigned code
with 0 indicating the union stores not value and a non-zero value
indicating the type of the stored union.

A union is stored in a table as a normal sub-table reference with the
only difference being that the offset does not always point to a table
of the same type. The 8-bit union type is stored as a separate table
field conventially named the same as the union value field except for a
`_type` suffix. The value (storing the table offset) MUST have a field
ID that is exactly one larger than the type field. If value field is
present the type field MUST also be present. If the type is NONE the
value field MUST be absent and the type field MAY be absent because a
union type always defaults to the value NONE.

Vectors of unions is a late addition to the FlatBuffers format. FlatCC
does currently not suppport them but they will be added eventually.

Vectors of unions have the same two fields as normal unions but they
both store a vector and both vectors MUST have the same length or both
be absent from the table. The type vector is a vector of 8-bit enums and
the value vector is a vector of table offsets. Obviously each type
vector element represents the type of the table in the corresponding
value element. If an element is of type NONE the value offset must be
stored as 0 which is a circular reference. This is the only offset that
can have the value 0.


## Struct Buffers

The FlatBuffers format does not permit structs to be stored as
independent entities. They are always embedded in in a
fixed memory block representing a table, in a vector, or embedded inline
in other structs.

The root object in FlatBuffers is expected to be a table, but it can
also be struct. FlatCC supports struct as root. Since structs do not
contain references, such buffers are truly flat. Most implementations
are not likely to support structs are root but even so they are very
useful:

It is possible to create very compact and very fast buffers this way.
They can be used where one would otherwise consider using manual structs
or memory blocks but with the advantage of a system and language
independent schema.

Structs as roots may be particularly interesting for the Big Endian
variant of FlatBuffers for two reasons: the first being that performance
likely matters when using such buffers and thus Big Endian platforms
might want them. The second reason is the network byte order is
traditionally big endian, and this has proven very difficult to change,
even in new evolving IETF standards. FlatBuffers can be used to manage
non-trival big endian encoded structs, especially structs containing
other structs, even when the receiver does not understand FlatBuffers as
concept - here the struct would be be shipped without the buffer header
as is.


## Alignment

All alignments are powers of two between 1 and 256. Large alignments are
only possible via schema specified alignments. The format naturally has
a maximum alignment of the largest scalar it stores, which is a 64-bit
integer or floating point value. Because C malloc typically returns
buffers aligned to a least 8 bytes, it is often safe to place buffers in
heap allocated memory, but if the target system does not permit
unaligned access, or is slow on unaligned access, a buffer should be
placed in sufficiently aligned memory. Typically it is a good idea to
place buffer in cacheline aligned boundary anyway.

A buffers alignment is the same as the largest alignment of any
object or struct it contains.

The buffer size is not guaranteed to be aligned to its own alignment
unlike struct. Googles `flatc` builder does this, at least when size
prefixed. The `FlatCC` tool currently does not, but it might later add
an option to pad up to alignment. This would make it simpler to stack
similar typed buffers in file - but users can retrieve the buffers
alignment and do this manually. Thus, when stacking size prefixed
buffers, each buffer should start aligned to its own size starting at
the size field, and should also be zero padded up to its own alignment.


## Default Values and Deprecated Values

A table can can omit storing any field that is not a required field. For
strings, vectors and tables this result in returning a null value
different from an empty value when reading the buffer. Struct fields not
present in table are also returned as null.

All fields that can return null do not have a default value. Other
values, which are integers, floats and enumerations, can all have
default values. The default value is returned if not found in the table.

If a default value is not specified the default defaults to zero for the
corresponding type. If an enumeration does not have a 0 value and no
explicit default value, this is a schema error.

When building a buffer the builder will compare a stored field to the
known default value. If it matches the field will simple be skipped.
Some builder API's makes it possible to force a default value to be
stored and to check if a field is missing when reading the buffer. This
can be used to handle NULL values differently from default or missing
values.

A deprecated field should be treated as not available, as in no way to
read the value as opposed to returning a default value. If they for some
reason are made accessible the verifier must also understand and verify
these fields.

Structs cannot have default values and cannot have deprecated fields in
stadard FlatBuffers. FlatCC supports marking a struct field as
deprecated. This implies the field will always be zeroed and with no
trivial accessors. A struct can never change size without breaking
support for schema evolution.

FlatCC JSON parsers allow structs to only set some values. Remaining
values will be implicitly zeroed. The C API for writing buffers do not
have this concern because a struct can just be written as a C struct
so there is no control over which fields a user choose to set or zero.
However, structs should be zeroed and padded where data is not otherwise
set. This makes it possible to hash and integrity check structs (though
this is not an integral part of the format).


## Schema Evolution

A table has a known set of low-valued field identifiers. Any unused
field id can be used in a future version. If a field (as is normal) is
implicitly assigned an id, new fields can only be added at the end of
the table. Internally this translates into new versions getting ever
larger vtables. Note that vtables are only stored as large as needed for
the actual content of table, so a rarely used new field will not cause
vtables to grew when unused.

Enumarations may not change values in future versions. Unions may only
added new table names to the end of the union list.

Structs cannot change size nor content. The cannot evolve. FlatCC
permits deprecating fields which means old fields will be zeroed.

Names can be changed to the extend it make sense to the applications already
written around the schema.

New types can be added.


## Keys and Sorting

Keys and sorting is a meta construct driven by the schema. The binary
format has no special concept of keys and sorting and a vector can be
sorted by one of several keys so it makes no sense to enforce a specific
order.

The basic FlatBuffers format only permit at most one key and generally
sorts vectors by that key during buffer construction. FlatCC does not do
this both because sorting is not practical while building the buffer and
because FlatCC supports sorting by one of several keys. Thus, in general
it is not safe to assume that a vector is sorted, but it can be sorted
if needed.

## Size Limits

A buffer should never be larger than `2^(sizeof(soffset_t) * 8 - 1) - 1`
or `2^31 - 1` i.e. 2GB for standard FlatBuffers. Beyond this safe it is
not safe to represent vtable offsets and implementations can no longer
use signed types to store `uoffset_t` values. This limit also ensures
that all vectors can be represented safely with a signed 32-bit length
type.

The application interface is likely to use a native type for
representing sizes and vector indices. If this type is smaller that
`sizeof(soffset_t)` or equivalently `sizeof(uoffset_t)` there is a risk
of overflow. The simplest way to avoid any issues is to limit the
accepted buffer size of the native size type. For example, on some
embedded microprocessor systems a C compiler might have a 16-bit int and
`size_t` type, even if it supports `uint32_t` as well. In this the safe
assumption is to limit buffers to `2^15 - 1` which is very likely more
than sufficient on such systems.

A builder API might also want to prevent vectors from being created when
they cannot stay within the size type of the platform when the element
size is multipled by the element count. This is deceiving because the
element count might easily be within range. Such issues will be rare in
praxis but they can be the source of magnificent vulnerabilites if not
handled appropriately.


## Verification

Verification as discussed here is not just about implementing a
verifier. It is as much requirements that any builder must fulfill.

The objective of verification is to make it safe to read from an
untrusted buffer, or a trusted buffer that accidentally has an
unexpected type. The verifier does not guarantee that the type is indeed
the expeced type exact it can read the buffer identifier if present
which is still no guarantee. The verifier cannot make it safe to modify
buffers in-place because the cost of doing such a check would be
prohitive in the average case.

A buffer verifier is expected to verify that all objects (strings,
vectors and tables) do not have an end position beyond the externally
specified buffer size and that all offset are aligned relative to offset
zero, and sometimes also relative to actually specified buffer (but
sometimes it is desireable to verify buffers that are not stored
aligned, such as in network buffers).

A verifier primarily checks that:

- the buffer is at least `2 * sizeof(uoffset_t)` such that the root
  root table and buffer identifier can be checked safely.
- any offset being chased is inside the buffer that any data accessed
  from that resuling location is entirely inside the buffer and aligned
  notably the vtables first entry must be valid before the vtable can
  safely validate itself, but this also applies to table, string and
  vector fields.
- any uoffset has size of at least `sizeof(uoffse_t)` to aviod
  self-reference and is no larger than the largest positive soffset_t
  value of same size - this ensures that implementations can safely add
  uoffset_t even if converting to signed first. It also, incidentally,
  ensures compatibility with StreamBuffers - see below.
- vtable size is aligned and does not end outside buffer.
- vtable size is at least the two header fields
  (`2 * `sizeof(voffset_t)`).
- required table fields are present.
- recursively verify all known fields and ignore other fields. Unknown
  fields are vtable entries after the largest known field ID of a table.
  These should be ignored in order to support forward versioning.
- deprecated fields are valid if accessors are available to do so, or
  ignore if the there is no way to access the field by application code.
- vectors end within the buffer.
- strings end within the buffer and has a zero byte after the end which
  is also within the buffer.
- table fields are aligned relative to buffer start - both structs,
  scalars, and offset types.
- table field size is aligned relative to field start.
- any table field does not end outside the tables size as given by the
  vtable.
- table end (without chasing offsets) is not outside buffer.
- all data referenced by offsets are also valid within the buffer
  according the type given by the schema.
- verify that recursion depth is limited to a configurable acceptable
  level for the target system both to protect itself and such that
  general recursive buffer operations need not be concerned with stack
  overflow checks (a depth of 100 or so would normally do).
- verify that if the union type is NONE the table field is absent and
  if it is not NONE that the table field is present. If the union type
  is known, the table should be verified. If the type is not known
  the table field should be ignored. A reader using the same schema would
  see the union as NONE. An unknown union is not an error in order to
  support forward versioning.


A verifier needs to be very careful in how it deals with overflow and
signs. Vector elements multiplied by element size can overflow. Adding
an invalid offset might become valid due to overflow. In C math on
unsigned types yield predictable two's complement overflow while signed
overflow is undefined behavior and can and will result in unpredictable
values with modern optimizing compilers. The upside is that if the
verifier handles all this correctly, the application reader logic can be
much simpler while staying safe.


A verifier does __not__ enforce that:

- structs and other table fields are aligned relative to table start because tables are only
  aligned to their soffset field. This means a table cannot be copied
  naively into a new buffer even if it has no offset fields.
- the order of individual fields within a table. Even if a schema says
  something about ordering this should be considered advisory. A
  verifier may additionally check for ordering for specific
  applications. Table order does not affect safety nor general buffer
  expectations. Ordering might affect size and performance.
- sorting as specified by schema attributes. A table might be sorted in
  different ways and an implementation might avoid sorting for
  performance reasons and other practical reasons. A verifier may,
  however, offer additional verification to ensure specific vectors or
  all vectors are sorted according to schema or other guidelines.  Lack
  of sorting might affect expected behavior but will not make the buffer
  unsafe to access.
- that structures do not overlap. Overlap can result in vulnerabilities
  if a buffer is modified, and no sane builder should create overlaps
  other than proper DAGs except via a separate compression/decopression
  stage of no interest to the verifier.
- strings are UTF-8 compliant. Lack of compliance may affect expections
  or may make strings appear shorter or garbled. Worst case a naive
  UTF-8 reader might reach beyond the end when observing a lead byte
  suggest data after buffer end, but such a read should discover the
  terminal 0 before things get out of hand. Being relaxed permits
  specific use cases where UTF-8 is not suitable without giving up the
  option to verify buffers. Implementations can add additional
  verification for specific usecases for example by providing a
  strict-UTF8 flag to a verifier or by verifying at the application
  level. This also avoids unnecessary duplicate validation, for example
  when an API first verifies the buffer then converts strings to an
  internal heap representation where UTF-8 is validated anyway.
- default values are not stored. It is common to force default values to
  be stored. This may be used to implement NULL values as missing
  values different from default values.
- enum values are only among the enum values of its type. There are many
  use cases where it is convenient to add enum values for example flags
  or enums as units e.g. `2 * kilo + 3 * ounce. More generally ordinary
  integers may have value range restrictions which is also out of scope
  for verifier. An application may provide additional verification when
  requested.

More generally we can say that a verifier is a basic fast assurance that
the buffer is safe to access. Any additional verification is application
specific. The verifier makes it safe to apply secondary validation.
Seconary validation could be automated via schema attributes and may be
very useful as such, but it is a separate problem and out of scope for a
core binary format verifier.

A buffer identifier is optional so the verifier should be informed
whether an identifier must match a given id. It should check both ASCII
text and zero padding not just a string compare. It is non-trivial to
decide if the second buffer field is an identifier, or some other data,
but if the field does not match the expected identifier, it certainly
isn't what is expected.

Note that it is not entirely trivial to check vector lengths because the
element size must be mulplied by the stored element count. For large
elements this can lead to overflows.


## Risks

Because a buffer can contain DAGs constructed to explode exponentiall,
it can be dangerous to print JSON or otherwise copy content blindly
if there is no upper limit on the export size.

In-place modification cannot be trusted because a standard buffer
verifier will detect safe read, but will not detect if two objects are
overlapping. For example, a table could be stored inside another table.
Modifing one table might cause access to the contained table to go out
of bounds, for example by directing the vtable elsewhere.

The platform native integer and size type might not be able to handle
large FlatBuffers - see [Size Limits](#size-limits). 

Becaue FlatCC requires buffers to be sorted after builiding, there is
risk due to buffer modifications. It is not sufficient to verify buffers
after sorting because sorting is done inline. Therefore buffers must be
trusted or rewritten before sorting.


## Nested FlatBuffers

FlatBuffers can be nested inside other FlatBuffers. In concept this is
very simple: a nested buffer is just a chunk of binary data stored in a
`ubyte` vector, typically with some convenience methods generated to
access a stored buffer. In praxis it adds a lot of complexity. Either
a nested buffer must be created strictly separately and copied in as
binary data, but often users mix the two builder contexts accidentally
storing strings from one buffer inside another. And when done right, the
containing ubyte vector might not be aligned appropriately making it
invalid to access the buffer without first copying out of the containing
buffer except where unaligned access is permitted. Further, a nested
FlatBuffer necessarily has a length prefix because any ubyte vector has
a length field at its start. Therefore, size prefixed flatbuffers should
not normally be stored as nested buffers, but sometimes it is necessary
in order have the buffer properly aligned after extraction.

The FlatCC builder makes it possible to build nested FlatBuffers while
the containing table of the parent buffer is still open. It is very
careful to ensure alignment and to ensure that vtables are not shared
between the two (or more) buffers, otherwise a buffer could not safely
be copied out. Users can still make mistakes by storing references from
one buffer in another.

Still, this area is so complex that several bugs have been found.
Thus, it is advice to use nested FlatBuffers with some care.

On the other hand, nested FlatBuffers make it possible to trivially
extract parts of FlatBuffer data. Extracting a table would require
chasing pointers and could potentially explode due to shared sub-graphs,
if not handled carefully.


## Big Endian FlatBuffers

FlatBuffers are formally always little endian and even on big-endian
platforms they are reasonably efficient to access.

However it is possible to compile FlatBuffers with native big endian
suppport using the FlatCC tool. The procedure is out of scope for this
text, but the binary format is discussed here:

All fields have exactly the same type and size as the little endian
format but all scalars including floating point values are stored
byteswapped within their field size.

The buffer identifier is stored byte swapped if present. For example
the 4-byte "MONS" identifier becomes "SNOM" in big endian format. It is
therefore reasonably easy to avoid accidentially mixing little- and
big-endian buffers. However, it is not trivial to handle identifers that
are not exactly 4-bytes. "HI\0\0" could be "IH\0\0" or "\0\0IH". It is
recommended to always use 4-byte identifies to avoid this problem. See
the FlatCC release 0.4.0 big endian release for details.

When using type hash identifiers (a fully qualified table name such
subjected to FNV-1A 32-bit hash), the user code will see the same
identifier for both little and big endian buffers but the accessor code
to accept the identifier will do the appropriate endian conversion. The
type hash is for example FNV1a_32("MyGame.Example.Monster") to get the
id of buffer storing the monster table as root, using the ubiquitious
`monster.fbs` example.


## StreamBuffers

StreamBuffers are so far only a concept although some implementations
may already be able to read them. The format is documented to aid
possible future implementations.

StreamBuffers makes it possible to store partially completed buffers
for example by writing directly to disk or by sending partial buffer
data over a network. Traditional FlatBuffers require an extra copying
step to make this possible, and if the writes are partial, each section
written must also store the segment length to support reassembly.
StreamBuffers avoid this problem.

Some may like the idea that the root table is stored first but modern
CPU cache systems do not care much about the order of access as long as
as their is some aspect of locality of reference and the same applies to
disk access while network access likely will have the end of the buffer
in hot cache as this is the last sent. The average distance between
objects will be the same for both FlatBuffers and StreamBuffers.

StreamBuffers treat `uoffset_t` the same as `soffset_t` when the special
limitation that `uoffset_t` is always negative when viewed as two's
complement values.

The implication is that everthing a table or vector references must be
stored earlier in the buffer, except vtables that can be stored freely.
Existing reader implementations that treat `uoffset_t` as signed, such as
Javascript, will be able to read StreamBuffers with no modification.
Other readers can easily be modified by casting the uoffset value to
signed before it.

Verifiers must ensure that any buffer verified always stores all
`uoffset_t` with the same sign. This ensures the DAG structure is
preserved without cycles.

The root table offset remains positive as an exception and points to the
root buffer. A stream buffer root table will be the last table in the
buffer.

The root table offset may be replaced with a root table offset at the
end of the buffer instead of the start. An implementation may also
zero the initial offset and update it later. In either case the buffer
should be aligned accordingly.

