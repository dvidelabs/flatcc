NOTE: shell scripts driven by flatcc/test/test.sh have been ported to CMake.
use flatcc/scripts/test.sh to drive CMake tests.

Run `leakcheck.sh` and `leakcheck-full.sh` for memory checks.

To install valgrind on OS-X Yosemite use `brew install --HEAD valgrind`

For decoding valgrind error messages:
<http://derickrethans.nl/valgrind-null.html>

clang has built-in memory check, but only for `x86_64 Linux`:
<http://clang.llvm.org/docs/LeakSanitizer.html>

On OS-X Yosemite with valgrind that isn't officially supported for that
platform, a few spurious unitialized memory access errors are reported
when printing the filextension in `codegen_c.c`. and in the equivalent
builder. After inspection, nothing suggests this is an actual bug - more
likely it relates to a strnlen optimization in fprintf `"%.*s"` syntax
that valgrind doesn't catch.
