# Change Log

## [Unreleased]

- Add `FLATCC_RTONLY` and `FLATCC_INSTALL` build options.
- Fix issue4: when building a buffer and the first thing
  created is an empty table, the builder wrongly assumed
  allocation failure. Affects runtime library.

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
