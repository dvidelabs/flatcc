#!/usr/bin/env sh

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

DBGDIR=$ROOT/build/Debug
RELDIR=$ROOT/build/Release

if [ "$1" = "--debug" ]; then
    DEBUG=$1
    echo "running debug build"
    shift
fi

if [ "$1" != "--no-clean" ]; then
    echo "cleaning build before tests ..."
    $ROOT/scripts/cleanall.sh
else
    shift
fi

echo "building before tests ..."
$ROOT/scripts/build.sh $DEBUG

echo "running test in debug build ..."
cd $DBGDIR && $FLATCC_TEST_CMD

if [ "$DEBUG" != "--debug" ]; then

  echo "running test in release build ..."
  cd $RELDIR && $FLATCC_TEST_CMD
  echo "TEST PASSED"
else
    echo "DEBUG TEST PASSED"
fi

