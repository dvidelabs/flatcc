#!/bin/sh

scripts/meson-debug-test.sh && scripts/meson-release-test.sh
# this breaks if build with dynamic library
build/release/test/benchmark/benchflatcc/benchflatcc
