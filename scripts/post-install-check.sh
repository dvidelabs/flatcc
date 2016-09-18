#!/bin/sh

# This script is not intended for windows, but the same concept applies.

set -e

CC=${CC:-cc}

flatcc --version

SMOKE=$TMPDIR/flatccsmoke

rm -rf $SMOKE
mkdir -p $SMOKE

flatcc -a test/monster_test/monster_test.fbs -o $SMOKE
$CC -lflatccrt test/monster_test/monster_test.c -I $SMOKE -o $SMOKE/monster
$SMOKE/monster

flatcc -a test/monster_test/monster_test.fbs -o $SMOKE
$CC -lflatccrt_sha test/monster_test/monster_test.c -I $SMOKE -o $SMOKE/monster_sha
$SMOKE/monster_sha

rm -rf $SMOKE
