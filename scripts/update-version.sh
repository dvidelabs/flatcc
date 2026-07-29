#!/usr/bin/env bash
set -euo pipefail

FILE="include/flatcc/flatcc_version.h"

usage()
{
    echo "usage: $0 VERSION"
    echo "example: $0 0.6.3-pre"
    exit 1
}

[ $# -eq 1 ] || usage

VERSION="$1"

if [[ ! "$VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)(-.*)?$ ]]; then
    echo "Invalid version: $VERSION"
    exit 1
fi

MAJOR="${BASH_REMATCH[1]}"
MINOR="${BASH_REMATCH[2]}"
PATCH="${BASH_REMATCH[3]}"
SUFFIX="${BASH_REMATCH[4]:-}"

if [ -z "$SUFFIX" ]; then
    RELEASED=1
else
    RELEASED=0
fi

sed -i.bak \
    -e "s/^#define FLATCC_VERSION_TEXT .*/#define FLATCC_VERSION_TEXT \"$VERSION\"/" \
    -e "s/^#define FLATCC_VERSION_MAJOR .*/#define FLATCC_VERSION_MAJOR $MAJOR/" \
    -e "s/^#define FLATCC_VERSION_MINOR .*/#define FLATCC_VERSION_MINOR $MINOR/" \
    -e "s/^#define FLATCC_VERSION_PATCH .*/#define FLATCC_VERSION_PATCH $PATCH/" \
    -e "s/^#define FLATCC_VERSION_RELEASED .*/#define FLATCC_VERSION_RELEASED $RELEASED/" \
    "$FILE"

rm "$FILE.bak"

echo "Updated $FILE:"
grep FLATCC_VERSION "$FILE"
