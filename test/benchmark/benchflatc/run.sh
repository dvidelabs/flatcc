#!/usr/bin/env bash

set -e
cd `dirname $0`/../../..
ROOT=`pwd`
TMP=build/tmp/test/benchmark/benchflatc
INC=$ROOT/include
mkdir -p ${TMP}
rm -rf ${TMP}/*

CXX=${CXX:-c++}
cp -r test/benchmark/benchmain/* ${TMP}
cp -r test/benchmark/benchflatc/* ${TMP}
#include include at root as it may conflict
cp -r ${ROOT}/include/flatcc/support ${TMP}

cd ${TMP}
$CXX -g -std=c++11 benchflatc.cpp -o benchflatc_d -I $INC
$CXX -O3 -DNDEBUG -std=c++11 benchflatc.cpp -o benchflatc -I $INC
echo "running flatbench flatc for C++ (debug)"
./benchflatc_d
echo "running flatbench flatc for C++ (optimized)"
./benchflatc
