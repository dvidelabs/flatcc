#!/usr/bin/env bash

set -e
cd `dirname $0`/../../..
ROOT=`pwd`
TMP=build/tmp/test/benchmark/benchraw
INC=$ROOT/include
mkdir -p ${TMP}
rm -rf ${TMP}/*

CC=${CC:-cc}
cp -r test/benchmark/benchmain/* ${TMP}
cp -r test/benchmark/benchraw/* ${TMP}

cd ${TMP}
$CC -g benchraw.c -o benchraw_d -I $INC
$CC -O3 -DNDEBUG benchraw.c -o benchraw -I $INC
echo "running flatbench for raw C structs (debug)"
./benchraw_d
echo "running flatbench for raw C structs (optimized)"
./benchraw
