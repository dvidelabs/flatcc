# Change Log

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
