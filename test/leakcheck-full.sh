#!/usr/bin/env bash

set -e
../build.sh
cd `dirname $0`/..
mkdir -p build/tmp/leakcheck-full
valgrind --leak-check=full --show-leak-kinds=all \
    bin/flatcc_d -a -o build/tmp/leakcheck-full --prefix zzz --common-prefix \
    hello test/monster_test/monster_test.fbs
