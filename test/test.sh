#!/usr/bin/env bash


echo "This is the old test script replaced by CMake's ctest"
echo "driven by scritps/test.sh"
echo "pausing 5 seconds - press ctrl+C to quit"

sleep 5

set -e
cd `dirname $0`/..
ROOT=`pwd`

CC=${CC:-cc}
${ROOT}/scripts/build.sh

TMP=${ROOT}/build/tmp/test
INC=${ROOT}/include

echo "" 1>&2

mkdir -p ${TMP}
rm -rf ${TMP}/*

echo "running generation of complex schema (cgen_test)"
${ROOT}/test/cgen_test/cgen_test.sh

mkdir -p ${TMP}/monster_test

mkdir -p ${TMP}/monster_test_smoke
mkdir -p ${TMP}/monster_test_solo
mkdir -p ${TMP}/monster_test_hello
mkdir -p ${TMP}/monster_test_main

#
# These first tests are to ensure the generated code can compile,
# they don't actually run tests against the api.
#
echo "generating smoke test generated monster source" 1>&2
${ROOT}/bin/flatcc -I ${ROOT}/test/monster_test -a \
    -o ${TMP}/monster_test_smoke ${ROOT}/test/monster_test/monster_test.fbs
echo "#include \"monster_test_builder.h\"" > ${TMP}/monster_test_smoke/smoke_monster.c
cd ${TMP}/monster_test_smoke && $CC -c -Wall -O3 -I ${INC} smoke_monster.c

echo "generating smoke test generated monster source to single file" 1>&2
${ROOT}/bin/flatcc -I ${ROOT}/test/monster_test -a --stdout \
    ${ROOT}/test/monster_test/monster_test.fbs > ${TMP}/monster_test_solo/solo_monster.c
cd ${TMP}/monster_test_solo && $CC -c -Wall -O3 -I ${INC} solo_monster.c

echo "generating smoke test generated monster source with --prefix zzz --common-prefix hello" 1>&2
${ROOT}/bin/flatcc -I ${ROOT}/test/monster_test -a \
    --common-prefix hello --prefix zzz \
    -o ${TMP}/monster_test_hello ${ROOT}/test/monster_test/monster_test.fbs
echo "#include \"monster_test_builder.h\"" > ${TMP}/monster_test_hello/hello_monster.c
cd ${TMP}/monster_test_hello && $CC -c -Wall -O3 -I ${INC} hello_monster.c

#
# This test ensures the reader api understands a monster buffer generated
# by the external `flatc` tool by Google FPL.
#
echo "starting compat test"
${ROOT}/test/flatc_compat/flatc_compat.sh

echo "starting emit_test for altenative emitter backend smoke test"
${ROOT}/test/emit_test/emit_test.sh

#
# This is the main `monster_test.c` test that covers nearly all
# functionality of the reader and builder API for C.
#
echo "running main monster test"
cd ${ROOT}/test/monster_test
${ROOT}/bin/flatcc -I ${ROOT}/test/monster_test -a \
    -o ${TMP}/monster_test_main ${ROOT}/test/monster_test/monster_test.fbs
cd ${TMP}/monster_test_main
cp ${ROOT}/test/monster_test/monster_test.c .
$CC -g -I ${ROOT}/include monster_test.c \
    ${ROOT}/lib/libflatccrt_d.a -o monster_test_d
$CC -O3 -DNDEBUG -DFLATBUFFERS_BENCHMARK -I ${ROOT}/include monster_test.c \
    ${ROOT}/lib/libflatccrt.a -o monster_test
./monster_test_d

echo "running optimized version of main monster test"
./monster_test

# This may fail if reflection feature is disabled
echo "running reflection test"
${ROOT}/test/reflection_test/reflection_test.sh

# This may fail if reflection feature is disabled
echo "running reflection sample"
${ROOT}/samples/reflection/build.sh

echo "running monster sample"
${ROOT}/samples/monster/build.sh

echo "running json test"
${ROOT}/test/json_test/json_test.sh

echo "running load test with large buffer"
${ROOT}/test/load_test/load_test.sh

echo "TEST PASSED"
