#!/bin/sh

set -e

HERE=`dirname $0`
cd $HERE/..
ROOT=`pwd`

CFGFILE=${ROOT}/scripts/build.cfg

if [ -e $CFGFILE ]; then
    . $CFGFILE
else
    # Default build system is cmake/ninja
    ${ROOT}/scripts/initbuild.sh cmake
    . $CFGFILE
fi

mkdir -p ${ROOT}/bin
mkdir -p ${ROOT}/lib

rm -f ${ROOT}/bin/flatcc
rm -f ${ROOT}/bin/flatcc_d
rm -f ${ROOT}/libflatcc
rm -f ${ROOT}/libflatcc_d.a
rm -f ${ROOT}/libflatccrt.a
rm -f ${ROOT}/libflatccrt_d.a

if [ ! -d ${ROOT}/build/Debug ] || [ ! -d ${ROOT}/build/Release ]; then
    ${ROOT}/scripts/initbuild.sh $FLATCC_BUILD_CONFIG
fi

echo "building Debug" 1>&2
cd ${ROOT}/build/Debug && $FLATCC_BUILD_CMD

if [ "$1" != "--debug" ]; then
    echo "building Release" 1>&2
    cd ${ROOT}/build/Release && $FLATCC_BUILD_CMD
fi
