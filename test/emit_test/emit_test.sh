#!/usr/bin/env bash

set -e
cd `dirname $0`/../..
ROOT=`pwd`
TMP=build/tmp/test
${ROOT}/scripts/build.sh

mkdir -p ${TMP}/emit_test
rm -rf ${TMP}/emit_test/*
bin/flatcc -a -o ${TMP}/emit_test test/emit_test/emit_test.fbs \
    test/monster_test/monster_test.fbs
cp test/emit_test/*.c ${TMP}/emit_test
cd ${TMP}/emit_test
cc -g -I ${ROOT}/include emit_test.c \
    ${ROOT}/lib/libflatccrt.a -o emit_test_d
echo "running emit test"
./emit_test_d

