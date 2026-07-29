#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

RPATH="include/flatcc/reflection"

mkdir -p "$RPATH"

bin/flatcc \
    -a \
    -o "$RPATH" \
    reflection/reflection.fbs

cp reflection/README.in "$RPATH/README"
