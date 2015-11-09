#!/usr/bin/env bash

set -e
cd `dirname $0`/..
mkdir -p build/tmp/out
lldb -- \
  bin/flatcc_d -a -o build/tmp/out --prefix zzz --common-prefix hello \
  test/monster_test/monster_test.fbs
