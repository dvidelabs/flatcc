#!/bin/sh

cd $(dirname $0)/..
scripts/test.sh
builddir=build/Debug
bfbsdir=$builddir/test/reflection_test/generated/
$builddir/samples/reflection/bfbs2json_d \
    $bfbsdir/monster_test.bfbs \
    | python -m json.tool > $bfbsdir/monster_test_bfbs.json

cat $bfbsdir/monster_test_bfbs.json
