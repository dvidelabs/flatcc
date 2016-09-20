#!/bin/sh
cd $(dirname $0)/..
set -e

BUILD_TYPE=${1:-debug}
MESON=$(scripts/get-meson.sh)
rm -rf build/$BUILD_TYPE && mkdir -p build/$BUILD_TYPE
$MESON build/$BUILD_TYPE --buildtype=release && ninja -C build/$BUILD_TYPE test
