# Change Log

## [0.5.0-pre]
- New schema type aliases: int8, uint8, int16, uint16, int32, uint32,
  int64, uint64, float32, float64.
- Low-level: access multiple user frames in builder via handles.
- Support for `_is_known_type` and `_is_known_value` on union and enum
  types.
- More casts for C++ compatiblity (#59).
- Fix regressions in verifier fix in 0.4.3 that might report out of
  bounds in rare cases (#60).
- Silence gcc 7.x warnings about implicit fallthrough (#61).
- Fix rare special in JSON parser causing spurious unknown symbol.
- Reading and writing union vectors. The C++ interface also supports
  these types, but other languages likely won't for a while.
- New `_union(t)` method for accessing a union fields type and member
  table in a single call. The method also supports union vectors to
  retrieve the type vector and member vector as a single object.
- BREAKING: In generated builder code for union references of the form
  `<union-name>_union_ref_t` the union members and the hidden `_member`
  field has been replaced with a single `member` field. Union
  constructors work as before: `Any_union_ref_t uref =
  Any_as_weapon(weapon_ref)` Otherwise use `.type` and `.member` fields
  directly. This change was necessary to support the builder API's new
  union vectors without hitting strict aliasing rules, for example as
  argument to `flatcc_builder_union_vector_push`. Expected impact: low
  or none. The physical struct layout remains unchanged.
- Fix off-by-one indexing in `flatbuffers_generic_table_vec_at`. Impact
  low since it was hardly relevant before union vectors were introduced.
- Add document on security considerations (#63).
- Add support for base64 and base64url attributes in JSON printing and
  parsing of [ubyte] table fields.

## [0.4.3]
- Fix issue with initbuild.sh for custom builds (#43)
- Add casts to aid clean C++ builds (#47)
- Add missing const specifier in generated `buffer_start` methods - removes C++
  warnings (#48)
- Update external/hash, removed buggy Sorted Robin Hood Hash that wasn't
  faster anyway - no impact on flatcc.
- Fix JSON parsing bug where some names are prefixes of others (#50).
- A Table of Contents in documentation :-)
- Move repetitive generated JSON string parsing into library.
- Add tests for JSON runtime compiled with different flags such as
  permitting unquoted keys.
- Fix building nested buffers when the parent buffer has not yet emitted
  any data (#51).
- Fix building nested buffers using the _nest() call (#52).
- Add `FLATCC_TRACE_VERIFY` as build option.
- Allow more costumization of allocation functions (#55).
- Avoid dependency on PORTABLE_H include guard which is too common (#55).
- (possibly breaking) Fix duplicate field check in flatcc_builder_table_add call.
- Fix incorrect infinity result in grisu3 parser and double to float
  overflow handling in parse_float in portable library (affects JSON
  of abnormal numeric values).
- Fix return value handling of parse_float, parse_double in JSON parser.
- Fix verifier vector alignment check - affects vectors with element size 8+.
- Fix missing static in generated enum and union functions in JSON
  printer (#57).

## [0.4.2]
- Fix SIGNIFICANT bug miscalculating the number of builder frames in
  use. Nesting 8 levels would cause memory corruption (#41).
- Fix minor memory leak in flatcc compiler.
- Reduce collisions in builders vtable hash.
- Remove broken dependency on `<mm_malloc.h>` for some GCC versions in
  `paligned_alloc.h` (#40).
- Allow C++ files to include `pstdalign.h` and `paligned_alloc.h` (#39).

## [0.4.1]
- Test for `posix_memalign` on GCC platforms and fix fallback
  `aligned_alloc`.
- Fix JSON parser handling of empty objects and tables.
- Fix JSON parser - some fields would not be accepted as valid (#17).
- Fix rare uncompilable doc comment in schema (#21).
- Avoid crash on certain table parser error cases (#30).
- Add support for scan similar to find in reader API, but for O(N)
  unsorted search, or search by a secondary key, and in sub-ranges.
- Optionally, and by default, allow scan by any field (#29), not just keys.
- More compact code generation for reader (hiding scan methods).
- Use __flatbuffers_utype_t for union type in reader instead of uint8_t.
- Add unaligned write to punaligned for completeness.
- Promote use of `flatcc_builder_finalize_aligned_buffer` in doc and
  samples over `flatcc_builder_finalize_buffer`.
- Add scope counter to pstatic_assert.h to avoid line number conflicts.
- Fix compiler error/warning for negative enums in generated JSON parser (#35).
- Fix potential compiler error/warnings for large enum/defaults in
  generated reader/builder (#35).
- Fix tab character in C++ style comments (#34)
- Fix incorrect api usage in binary schema builder (#32)
- Support hex constants in fbs schema (flatc also supports these now) (#33).


## [0.4.0]
- Fix Windows detection in flatcc/support/elapsed.h used by benchmark.
- Fix #8 surplus integer literal suffix in portable byteswap fallback.
- Fix `pstatic_assert.h` missing fallback case.
- Fix #9 return values from allocation can be zero without being an error.
- Fix #11 by avoiding dependency on -lm (libmath) by providing a cleaner
  over/underflow function in `include/flatcc/portable/pparsefp.h`.
- Fix #12 infinite loop during flatbuffer build operations caused by
  rare vtable dedupe hash table collision chains.
- Added `include/flatcc/support/cdump.h` tool for encoding buffers in C.
- JSON code generators no longer use non-portable PRIszu print
  modifiers. Fixes issue on IBM XLC AIX.
- Deprecated support for PRIsz? print modifiers in
  `include/flatcc/portable/pinttypes.h`, they would require much more
  work to be portable.
- Fix and clean up `__STDC__` version checks in portable library.
- Improve IBM XLC support in `pstdalign.h`.
- Always include `pstdalign.h` in `flatcc_flatbuffers.h` because some
  C11 compilers fail to provide `stdalign.h`.
- Buffer verifier used to mostly, but not always, verify buffer
  alignment relative to buffer start. With size prefixed buffers it is
  necessary to verify relative to the allocated buffer, which is also
  safer and more consistent, but adds requirements to aligned allocation.
- `monster_test` and `flatc_compat` test now uses aligned alloc.
- Add `aligned_alloc` and `aligned_free` to `pstdalign.h`.
- `flatcc_builder_finalize_aligned_buffer` now requires `aligned_free`
  to be fully portable and no longer use unaligned malloc as fallback,
  but still works with `free` on most platforms (not Windows).

- BREAKING: Size prefixed buffers added requires a minor change
  to the low-level flatcc builder library with a flag argument to create
  and start buffer calls. This should not affect user code.


Changes related to big endian support which do not affect little endian
platforms with little endian wire format.

- Support for big endian platforms, tested on IBM AIX Power PC.
- Support for big endian encoded flatbuffers on both little and big
  endian host platforms via `FLATBUFFERS_PROTOCOL_IS_LE/BE` in
  `include/flatcc/flatcc_types.h`. Use `flatbuffers_is_native_pe()` to
  see if the host native endian format matches the buffer protocol.
  NOTE: file identifier at buffer offset 4 is always byteswapped.

In more detail:

- Fix vtable conversion to protocol endian format. This keeps cached
  vtables entirely in native format and reduces hash collisions and only
  converts when emitting the vtable to a buffer location.
- Fix structs created with parameter list resulting in double endian
  conversion back to native. 
- Fix string swap used in sort due to endian sensitive diff math. 
- Disable direct vector access test case when running on non-native
  endian platform.
- Update JSON printer test to handle `FLATBUFFERS_PROTOCOL_IS_BE`.
- Fix emit test case. Incorrect assumption on acceptable null pointer
  breaks with null pointer conversion. Also add binary check when
  `FLATBUFFERS_PROTOCOL_IS_BE`.
- Add binary test case to `json_test` when `FLATBUFFERS_PROTOCOL_IS_BE`.
- Fix endian sensitive voffset access in json printer.
- Update `flatc_compat` to reverse acceptance of 'golden' little endian
  reference buffer when `FLATBUFFERS_PROTOCOL_IS_BE`.

## [0.3.5a]
- Fix regression introduced in 0.3.5 that caused double memory free on
  input file buffer. See issue #7.

## [0.3.5]

- Allow flatcc cli options anywhere in the argument list.
- Add --outfile option similar to --stdout, but to a file.
- Add --depfile and --deptarget options for build dependencies.
- Allow some test cases to accept arguments to avoid hardcoded paths.
- Deprecate --schema-namespace=no option to disable namespace prefixes
  in binary schema as Google flatbuffers now also includes namespaces
  according to https://github.com/google/flatbuffers/pull/4025

## [0.3.4]

- Add `FLATCC_RTONLY` and `FLATCC_INSTALL` build options.
- Fix issue4: when building a buffer and the first thing created is an
  empty table, the builder wrongly assumed allocation failure. Affects
  runtime library.
- `scripts/setup.sh` now also links to debug libraries useful for bug
  reporting.
- Add ULL suffix to large printed constants in generated code which
  would otherwise required --std=c99 to silence warnings.

## [0.3.3]

- BREAKING: `verify_as_root` no longer takes an identifier argument, use
  `verify_as_root_with_identifier`. `myschema_verifier.h` now
  includes `myschema_reader.h` to access identifier.
  identifer argument, and variants for type identifiers;.
- Added scripts/setup.sh to quickly get started on small user projects.
- Support `namespace ;` for reverting to global namespace in schema.
- Enable block comments now that they are supported in flatc.
- Parse and validate new `rpc_service` schema syntax, but with no
  support for code generation.
- Add type hash support (`create/verify_as_typed_root` etc.) to
  optionally store and verify file identifiers based on hashed fully
  qualified type names.
- Fix potential issue with detection of valid file identifiers in
  buffer.
- Moved `include/support` into `include/flatcc/support`, renamed
  `include/support/readfile.h` function `read_file` to `readfile`.
- Make `FLATCC_TEST` build option skip building samples and test
  files, as opposed to just skip running the tests.
- `vec_at`, `vec_find`, etc. now use index type `size_t` instead of
  `flatbuffers_uoffset_t`.
- Removed `size_t` conversion warnings on Win64.

## [0.3.2]

- Move compiler warning handling from generated headers to portable
  library.
- Clean up warnings and errors for older gcc, clang and MSVC compilers.
- CI builds.
- Fix and improve portable version of `static_assert`.
- Add integer parsing to portable library.

## [0.3.1]

- Add support for MSVC on Windows.
- Allow FlatBuffer enums to be used in switch statements without warnings.
- Remove warnings for 32-bit builds.
- Fix runtime detection of endianness and add support for more
  platforms with compile time detection of endianness.
- Fix scope bug where global namespace symbols from included schema
  would be invisible in parent schema.
- Add missing `static` for generated union verifiers.
- Fix bug in json printer unicode escape and hash table bug in
  compiler.
- Fix json parser under allocation bug.
- Fix `emit_test` early dealloc bug.

## [0.3.0]

- Rename examples folder to samples folder.
- Add samples/monster example.
- BREAKING: added missing `_vec` infix on some operations related to
  building vectors. For example `Weapon_push` -> `Weapon_vec_push`.
- BREAKING: vector and string start functions no longer takes a
  count/len argument as it proved tedious and not very useful.
  The return value is now 0 on success rather than a buffer pointer.
  Use `_extend` call after start when the length argument is non-zero.

## [0.2.1]

- Disallow unquoted symbolic list in JSON parser by default for Google
  flatc compatibility.
- Remove PRIVATE flags from CMake build files to support older CMake
  versions.
- Simplify switching between ninja and make build tools.
- Fix incorrectly named unaligned read macros - impacts non-x86 targets.
- Mirror grisu3 headers in portable library to avoid dependency on
  `external/grisu3`.
- Use correct grisu3 header for parsing, improving json parsing times.
- Move `include/portable` to `include/flatcc/portable` to simplify
  runtime distribution and to prevent potential name and versioning
  conflicts.
- Fix `is_union` in bfbs2json.c example.

## [0.2.0]

- BREAKING: flatcc verify functions now return a distinct error code.
  This breaks existing code. Before non-zero was success, now `flatcc_verify_ok` == 0.
  The error code can be converted to a string using `flatcc_verify_error_string(ret)`.
- BREAKING, minor: Remove user state from builder interface - now
  providing a user stack instead.
- Fix verification of nested flatbuffers.
- Fix certain header fields that was not endian encoded in builder.
- MAJOR: Generate json printer and parser.
- Added high performance integer printinger to portable library
  and fast floating point priting to runtime library (grisu3) for JSON.
- Comparison agains default value now prints float to generated source
  with full precision ("%.17g").
- MAJOR: read-only generated files no longer attempt to be independent
  of files in the flatcc include dir. Instead they will use one
  well-defined source of information for flatbuffer types and endian detection.
- Always depend `portable/pendian.h` and `portable/pendian_detect.h`.
  (The `include/portable` dir can be copied under `include/flatcc` if so desired).
- Updates to set of include files in include/flatcc.
- Upgrade to pstdint.h 0.1.15 to fix 64-bit printf issue on OS-X.
- Support for portable unaligned reads.
- Hide symbols that leak into namespace via parent include.
- Suppress unused function and variable warnings for GCC (in addition to clang).

## [0.1.1]

- Rename libflatccbuilder.a to libflatccrt.a (flatcc runtime).
- Add suffix to all generated files (`monster_test.h -> monster_test_reader.h`)
- Add buffer verifiers (`monster_test_verifier.h).
- Assert on error in flatcc builder by default.
- Fix -I include path regression in `flatcc` command.

## [0.1.0]

- Initial public release.
