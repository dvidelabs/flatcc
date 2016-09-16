#!/bin/bash
cd "${0%/*}/.."

rm -rf build && mkdir -p build/release && \
    meson.py build/release --buildtype=release && ninja -C build/release test
