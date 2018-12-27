#!/bin/sh

#export CC=clang
#export CXX=clang++

# Note: flatcc's cmake seems to pretty much ignore all the standard cmake options, so we end up with
#       everything in lib/, include/, etc. rather than build/linux64.
CMAKE_CMD='cmake -DFLATCC_DEBUG_VERIFY=1 -D CMAKE_INSTALL_PREFIX="../linux64" -G "Unix Makefiles" -D CMAKE_DEBUG_POSTFIX="_debug"'

mkdir -p build/lindebug
cd build/lindebug
eval $CMAKE_CMD -D CMAKE_BUILD_TYPE="Debug" ../..
make -j7
cd ../..

#mkdir -p build/linrelease
#cd build/linrelease
#eval $CMAKE_CMD -D CMAKE_BUILD_TYPE="Release" ../..
#make -j7
#cd ../..
