#!/bin/sh

# Note that the benchmark won't run if it is linked dynamically because
# the load path is not setup.

cd $(dirname $0)/..

scripts/meson-setup.sh debug
build/debug/test/benchmark/benchflatcc/benchflatcc

scripts/meson-setup.sh release
build/release/test/benchmark/benchflatcc/benchflatcc
