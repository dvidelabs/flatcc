#!/usr/bin/env bash

set -e

echo "initializing build for CMake Ninja"

cd `dirname $0`/..
ROOT=`pwd`

mkdir -p ${ROOT}/build/Debug/*
mkdir -p ${ROOT}/build/Release
rm -rf ${ROOT}/build/Debug/*
rm -rf ${ROOT}/build/Debug/*

cd ${ROOT}/build/Debug && cmake -GNinja ../.. -DCMAKE_BUILD_TYPE=Debug
cd ${ROOT}/build/Release && cmake -GNinja ../.. -DCMAKE_BUILD_TYPE=Release
