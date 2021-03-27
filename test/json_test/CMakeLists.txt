set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

set(DATA_DST "${CMAKE_CURRENT_BINARY_DIR}")
set(DATA_SRC "${PROJECT_SOURCE_DIR}/test/flatc_compat")

flatcc_generate_sources(
    NAME json_test
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL VERIFIER JSON
    OUTPUT_DIR "${GEN_DIR}"
)

add_custom_command (OUTPUT "${DATA_DST}/monsterdata_test.golden" "${DATA_DST}/monsterdata_test.mon"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DATA_SRC}/monsterdata_test.golden" "${DATA_DST}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${DATA_SRC}/monsterdata_test.mon" "${DATA_DST}"
    DEPENDS "${DATA_SRC}/monsterdata_test.golden" "${DATA_SRC}/monsterdata_test.mon"
)
add_custom_target(copy_json_test_inputs ALL
    DEPENDS "${DATA_DST}/monsterdata_test.golden" "${DATA_DST}/monsterdata_test.mon")

add_executable(test_basic_parse test_basic_parse.c)
add_executable(test_json_parser test_json_parser.c)
add_executable(test_json_printer test_json_printer.c)
add_executable(test_json test_json.c)

target_link_libraries(test_basic_parse PRIVATE flatcc::runtime flatcc_generated::json_test )
target_link_libraries(test_json_parser PRIVATE flatcc::runtime flatcc_generated::json_test )
target_link_libraries(test_json_printer PRIVATE flatcc::runtime flatcc_generated::json_test )
target_link_libraries(test_json PRIVATE flatcc::runtime flatcc_generated::json_test )

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
    add_executable(${trg} test_json.c ${RTSRC})
    target_link_libraries(${trg} PRIVATE flatcc_generated::json_test)
    target_include_directories(${trg} PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME ${trg} COMMAND ${trg})
    set_target_properties(${trg} PROPERTIES COMPILE_FLAGS ${flags})
endmacro()

jstest(json_test_uql "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=1")
jstest(json_test_uql_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=0")
jstest(json_test_uq "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=1")
jstest(json_test_uq_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=0")
jstest(json_test "-DFLATCC_JSON_PARSE_WIDE_SPACE=1")
