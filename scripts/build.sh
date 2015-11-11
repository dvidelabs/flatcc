#!/usr/bin/env bash

set -e

cd `dirname $0`/..
ROOT=`pwd`

mkdir -p ${ROOT}/bin
mkdir -p ${ROOT}/lib

rm -f ${ROOT}/bin/flatcc{,_d}
rm -f ${ROOT}/libflatcc{,_d}.a
rm -f ${ROOT}/libflatccrt{,_d}.a

if [ ! -d ${ROOT}/build/Debug ] || [ ! -d  ${ROOT}/build/Release ]; then
    ${ROOT}/scripts/initbuild.sh
fi

echo "building Debug" 1>&2
cd ${ROOT}/build/Debug && ninja

echo "building Release" 1>&2
cd ${ROOT}/build/Release && ninja
