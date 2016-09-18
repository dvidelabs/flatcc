#!/bin/sh

# This script is not intended for windows, but the same concept applies.

set -e

echo running flatcc post install check

CC=${CC:-cc}

flatcc --version || \
    echo flatcc executable not available for post-install check - might be a path issue

SMOKE=$TMPDIR/flatccsmoke

rm -rf $SMOKE
mkdir -p $SMOKE

flatcc -a test/monster_test/monster_test.fbs -o $SMOKE
$CC -lflatccrt test/monster_test/monster_test.c -I $SMOKE -o $SMOKE/monster
$SMOKE/monster

rm -rf $SMOKE

echo flatcc post install check completed successfully
