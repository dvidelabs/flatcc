#!/bin/sh
set -e
HERE=$(dirname $0)
ROOT=$HERE/..
BUILD=$ROOT/build/meson-dist
rm -rf $BUILD
mkdir -p $BUILD
mkdir -p $BUILD
meson $BUILD --buildtype=release
ninja -C $BUILD test
ninja -C $BUILD flatbench
ninja -C $BUILD install
$HERE/post-install-check.sh
rm -rf $BUILD
