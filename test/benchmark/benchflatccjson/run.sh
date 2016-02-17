#!/usr/bin/env bash

set -e
cd `dirname $0`/../../..
ROOT=`pwd`
TMP=build/tmp/test/benchmark/benchflatccjson
${ROOT}/scripts/build.sh
mkdir -p ${TMP}
rm -rf ${TMP}/*
bin/flatcc --json -crw -o ${TMP} test/benchmark/schema/flatbench.fbs

CC=${CC:-cc}
cp -r test/benchmark/benchmain/* ${TMP}
cp -r test/benchmark/benchflatccjson/* ${TMP}
cd ${TMP}
$CC -g -std=c11 -I ${ROOT}/include benchflatccjson.c \
    ${ROOT}/lib/libflatccrt_d.a -o benchflatccjson_d
$CC -O3 -DNDEBUG -std=c11 -I ${ROOT}/include benchflatccjson.c \
    ${ROOT}/lib/libflatccrt.a -o benchflatccjson
echo "running flatbench flatcc json parse and print for C (debug)"
./benchflatccjson_d
echo "running flatbench flatcc json parse and print for C (optimized)"
./benchflatccjson
