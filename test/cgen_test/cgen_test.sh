#!/usr/bin/env bash

set -e
cd `dirname $0`/../..
ROOT=`pwd`

CC=${CC:-cc}
TMP=${ROOT}/build/tmp/test/cgen_test
INC=${ROOT}/include

${ROOT}/scripts/build.sh

mkdir -p ${TMP}
rm -rf ${TMP}/*

echo "generating test source for Debug" 1>&2
${ROOT}/build/Debug/test/cgen_test/cgen_test_d > ${TMP}/test_generated_d.c
cd ${TMP} && $CC test_generated_d.c -c -I${INC}

echo "generating test source for Release" 1>&2
${ROOT}/build/Release/test/cgen_test/cgen_test > ${TMP}/test_generated.c
cd ${TMP} && $CC test_generated.c -c  -Wall -O3 -I${INC}
