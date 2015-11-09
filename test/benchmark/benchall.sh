#!/usr/bin/env bash

set -e

cd `dirname $0`

echo "running all benchmarks (raw, flatc C++, flatcc C)\n"

echo "building and benchmarking raw strucs"
benchraw/run.sh
echo "building and benchmarking flatc generated C++"
benchflatc/run.sh
echo "building and benchmarking flatcc generated C"
benchflatcc/run.sh
