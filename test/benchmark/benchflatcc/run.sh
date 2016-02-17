#!/usr/bin/env bash

set -e
cd `dirname $0`/../../..
ROOT=`pwd`
TMP=build/tmp/test/benchmark/benchflatcc
${ROOT}/scripts/build.sh
mkdir -p ${TMP}
rm -rf ${TMP}/*
#bin/flatcc -a -o ${TMP} test/benchmark/schema/flatbench.fbs
bin/flatcc --json-printer -a -o ${TMP} test/benchmark/schema/flatbench.fbs

CC=${CC:-cc}
cp -r test/benchmark/benchmain/* ${TMP}
cp -r test/benchmark/benchflatcc/* ${TMP}
cd ${TMP}
$CC -g -std=c11 -I ${ROOT}/include benchflatcc.c \
    ${ROOT}/lib/libflatccrt_d.a -o benchflatcc_d
$CC -O3 -DNDEBUG -std=c11 -I ${ROOT}/include benchflatcc.c \
    ${ROOT}/lib/libflatccrt.a -o benchflatcc
echo "running flatbench flatcc for C (debug)"
./benchflatcc_d
echo "running flatbench flatcc for C (optimized)"
./benchflatcc
