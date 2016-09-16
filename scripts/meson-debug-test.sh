#!/bin/bash
cd "${0%/*}/.."

rm -rf build && mkdir -p build/debug && \
    meson.py build/debug --buildtype=debug && ninja -C build/debug test
