#!/usr/bin/env bash

set -e
cd `dirname $0`/../..
ROOT=`pwd`
TMP=build/tmp/test/load_test

${ROOT}/scripts/build.sh
mkdir -p ${TMP}
rm -rf ${TMP}/*
bin/flatcc -a -o ${TMP} test/monster_test/monster_test.fbs

cp test/load_test/*.c ${TMP}
cd ${TMP}
cc -g -I ${ROOT}/include load_test.c \
    ${ROOT}/lib/libflatccrt.a -o load_test_d
cc -O3 -DNDEBUG -I ${ROOT}/include load_test.c \
    ${ROOT}/lib/libflatccrt.a -o load_test
echo "running load test debug"
./load_test_d
echo "running load test optimized"
./load_test
