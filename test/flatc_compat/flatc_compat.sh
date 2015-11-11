#!/usr/bin/env bash

set -e
cd `dirname $0`/../..
ROOT=`pwd`
TMP=build/tmp/test

${ROOT}/scripts/build.sh

mkdir -p ${TMP}/flatc_compat
rm -rf ${TMP}/flatc_compat/*
bin/flatcc -a -o ${TMP}/flatc_compat test/monster_test/monster_test.fbs

cp test/flatc_compat/*.{json,mon,c} ${TMP}/flatc_compat/
cd ${TMP}/flatc_compat
cc -g -I ${ROOT}/include flatc_compat.c \
    ${ROOT}/lib/libflatccrt.a -o flatc_compat_d
echo "Google FPL flatc compatibility test - reading flatc generated binary"
./flatc_compat_d

