#!/usr/bin/env sh

set -e

HERE=`dirname $0`
cd $HERE/..
ROOT=`pwd`

DBGDIR=$ROOT/build/Debug
RELDIR=$ROOT/build/Release

if [ "$1" != "--no-clean" ]; then
echo "cleaning build before tests ..."
$ROOT/scripts/cleanall.sh
fi

echo "building before tests ..."
$ROOT/scripts/build.sh

echo "running test in debug build ..."
cd $DBGDIR && ctest $ROOT

echo "running test in release build ..."
cd $RELDIR && ctest $ROOT

echo "TEST PASSED"

if [ ! -e ${ROOT}/build/reflection_enabled ]; then
    echo "(reflection disabled, skipping affected test and example)"
fi

