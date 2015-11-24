# Change Log

## [Unreleased]

- Upgrade to pstdint.h 0.1.15 to fix 64-bit printf issue on OS-X.
- Remove user state from builder interface.
- Support for portable unaligned reads.

## [0.1.1]

- Rename libflatccbuilder.a to libflatccrt.a (flatcc runtime).
- Add suffix to all generated files (`monster_test.h -> monster_test_reader.h`)
- Add buffer verifiers (`monster_test_verifier.h).
- Assert on error in flatcc builder by default.
- Fix -I include path regression in `flatcc` command.

## [0.1.0]

- Initial public release.
