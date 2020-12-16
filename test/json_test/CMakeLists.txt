include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

set(DATA_DST "${CMAKE_CURRENT_BINARY_DIR}")
set(DATA_SRC "${PROJECT_SOURCE_DIR}/test/flatc_compat")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/attributes_builder.h" "${GEN_DIR}/attributes_json_parser.h"
        "${GEN_DIR}/attributes_json_printer.h" "${GEN_DIR}/attributes_reader.h" "${GEN_DIR}/attributes_verifier.h"
        "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/include_test1_builder.h" "${GEN_DIR}/include_test1_json_parser.h"
        "${GEN_DIR}/include_test1_json_printer.h" "${GEN_DIR}/include_test1_reader.h"
        "${GEN_DIR}/include_test1_verifier.h" "${GEN_DIR}/include_test2_builder.h"
        "${GEN_DIR}/include_test2_json_parser.h" "${GEN_DIR}/include_test2_json_printer.h"
        "${GEN_DIR}/include_test2_reader.h" "${GEN_DIR}/include_test2_verifier.h"
        "${GEN_DIR}/monster_test_builder.h" "${GEN_DIR}/monster_test_json_parser.h"
        "${GEN_DIR}/monster_test_json_printer.h" "${GEN_DIR}/monster_test_reader.h"
        "${GEN_DIR}/monster_test_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND cmake -E copy "${DATA_SRC}/monsterdata_test.golden" "${DATA_DST}"
    COMMAND cmake -E copy "${DATA_SRC}/monsterdata_test.mon" "${DATA_DST}"
    COMMAND flatcc::cli -av --json -o "${GEN_DIR}" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)

add_executable(test_basic_parse test_basic_parse.c)
add_executable(test_json_parser test_json_parser.c "${GEN_DIR}/monster_test_reader.h" "${GEN_DIR}/monster_test_json_parser.h")
add_executable(test_json_printer test_json_printer.c "${GEN_DIR}/monster_test_json_parser.h")
add_executable(test_json test_json.c "${GEN_DIR}/monster_test_json_parser.h" "${GEN_DIR}/monster_test_json_printer.h")

target_link_libraries(test_basic_parse PRIVATE flatcc::runtime)
target_link_libraries(test_json_parser PRIVATE flatcc::runtime)
target_link_libraries(test_json_printer PRIVATE flatcc::runtime)
target_link_libraries(test_json PRIVATE flatcc::runtime)

add_test(NAME test_basic_parse COMMAND test_basic_parse)
add_test(NAME test_json_parser COMMAND test_json_parser)
add_test(NAME test_json_printer COMMAND test_json_printer)
add_test(NAME test_json COMMAND test_json)

# Compile without default library in order to test various runtime flags
set(RTPATH "${PROJECT_SOURCE_DIR}/src/runtime")
set(RTSRC
    "${RTPATH}/builder.c"
    "${RTPATH}/emitter.c"
    "${RTPATH}/refmap.c"
    "${RTPATH}/verifier.c"
    "${RTPATH}/json_parser.c"
    "${RTPATH}/json_printer.c"
)

macro(jstest trg flags)
    add_executable(${trg} test_json.c ${RTSRC} "${GEN_DIR}/monster_test_json_parser.h"
        "${GEN_DIR}/monster_test_json_printer.h" "${GEN_DIR}/monster_test_verifier.h")
    target_include_directories(${trg} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME ${trg} COMMAND ${trg})
    set_target_properties(${trg} PROPERTIES COMPILE_FLAGS ${flags})
endmacro()

jstest(json_test_uql "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=1")
jstest(json_test_uql_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=0")
jstest(json_test_uq "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=1")
jstest(json_test_uq_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=0")
jstest(json_test "-DFLATCC_JSON_PARSE_WIDE_SPACE=1")
