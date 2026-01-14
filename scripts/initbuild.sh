#!/bin/sh

# link a specific build.cfg.xxx to build.cfg to use that build
# configuration, e.g. ln -sf build.cfg.make build.cfg
#
# call build/cleanall.sh before changing

set -x

HERE=`dirname $0`
cd $HERE/..
ROOT=`pwd`
cd $HERE

CFGFILE=${ROOT}/scripts/build.cfg

if [ x"$1" != x ]; then

    if [ -e ${CFGFILE}.$1 ]; then
        ln -sf ${CFGFILE}.$1 $CFGFILE
    else
        echo "missing config file for build generator option: $1"
        exit -1
    fi
    ${ROOT}/scripts/cleanall.sh
fi

if [ -e $CFGFILE ]; then
    . $CFGFILE
fi

mkdir -p ${ROOT}/build/Debug
mkdir -p ${ROOT}/build/Release
rm -rf ${ROOT}/build/Debug/*
rm -rf ${ROOT}/build/Release/*

echo "initializing build for $FLATCC_BUILD_SYSTEM $FLATCC_BUILD_GEN"

cd ${ROOT}/build/Debug && $FLATCC_BUILD_SYSTEM "$FLATCC_BUILD_GEN" $FLATCC_BUILD_FLAGS ../.. $FLATCC_TARGET_DEBUG
cd ${ROOT}/build/Release && $FLATCC_BUILD_SYSTEM "$FLATCC_BUILD_GEN" $FLATCC_BUILD_FLAGS ../.. $FLATCC_TARGET_RELEASE
