#!/usr/bin/env bash

cd `dirname $0`
mkdir -p "build"
cd build && cmake -GNinja .. && ninja
