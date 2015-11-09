#!/usr/bin/env bash

set -e
../build.sh
cd `dirname $0`/..
mkdir -p build/tmp/leakcheck
valgrind --leak-check=yes \
    bin/flatcc_d -a -o build/tmp/leakcheck --prefix zzz --common-prefix hello \
    test/monster_test/monster_test.fbs
