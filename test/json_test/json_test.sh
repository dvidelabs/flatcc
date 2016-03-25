#!/usr/bin/env bash

set -e
cd `dirname $0`/../..
ROOT=`pwd`
TMP=${ROOT}/build/tmp/test/json_test

CC=${CC:-cc}
${ROOT}/scripts/build.sh
mkdir -p ${TMP}
rm -rf ${TMP}/*

# could also use --json to generate both at once
bin/flatcc -av --json -o ${TMP} test/monster_test/monster_test.fbs

cp test/json_test/*.c ${TMP}
cp test/flatc_compat/monsterdata_test.golden ${TMP}
cp test/flatc_compat/monsterdata_test.mon ${TMP}

cd ${TMP}

$CC -g -I ${ROOT}/include test_basic_parse.c \
    ${ROOT}/lib/libflatccrt_d.a -o test_basic_parse_d

$CC -g -I ${ROOT}/include test_json_parser.c \
    ${ROOT}/lib/libflatccrt_d.a -o test_json_parser_d

$CC -g -I ${ROOT}/include test_json_printer.c \
    ${ROOT}/lib/libflatccrt_d.a -o test_json_printer_d

$CC -g -I ${ROOT}/include test_json.c\
    ${ROOT}/lib/libflatccrt_d.a -o test_json_d


echo "running json basic parse test debug"
./test_basic_parse_d

echo "running json parser test debug"
./test_json_parser_d

echo "running json printer test debug"
./test_json_printer_d

echo "running json test debug"
./test_json_d

$CC -O2 -DNDEBUG -I ${ROOT}/include test_basic_parse.c \
    ${ROOT}/lib/libflatccrt.a -o test_basic_parse

#$CC -O3 -DNDEBUG -I ${ROOT}/include test_json_parser.c \
#$CC -Os -save-temps -DNDEBUG -I ${ROOT}/include test_json_parser.c \

$CC -O2 -DNDEBUG -I ${ROOT}/include test_json_parser.c \
    ${ROOT}/lib/libflatccrt.a -o test_json_parser

$CC -O2 -DNDEBUG -I ${ROOT}/include test_json_printer.c\
    ${ROOT}/lib/libflatccrt.a -o test_json_printer

$CC -O2 -DNDEBUG -I ${ROOT}/include test_json.c\
    ${ROOT}/lib/libflatccrt.a -o test_json

echo "running json basic parse test optimized"
./test_basic_parse

echo "running json parser test optimized"
./test_json_parser

echo "running json printer test optimimized"
./test_json_printer

echo "running json test optimized"
./test_json

echo "json tests passed"
