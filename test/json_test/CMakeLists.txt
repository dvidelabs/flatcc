include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

set(DATA_DST "${CMAKE_CURRENT_BINARY_DIR}")
set(DATA_SRC "${PROJECT_SOURCE_DIR}/test/flatc_compat")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_monster_test_json ALL)
add_custom_command (
    TARGET gen_monster_test_json
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy "${DATA_SRC}/monsterdata_test.golden" "${DATA_DST}"
    COMMAND ${CMAKE_COMMAND} -E copy "${DATA_SRC}/monsterdata_test.mon" "${DATA_DST}"
    COMMAND flatcc_cli -av --json -o "${GEN_DIR}" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)

add_executable(test_basic_parse test_basic_parse.c)
add_executable(test_json_parser test_json_parser.c)
add_executable(test_json_printer test_json_printer.c)
add_executable(test_json test_json.c)

add_dependencies(test_basic_parse gen_monster_test_json)
add_dependencies(test_json_parser gen_monster_test_json)
add_dependencies(test_json_printer gen_monster_test_json)
add_dependencies(test_json gen_monster_test_json)

target_link_libraries(test_basic_parse flatccrt)
target_link_libraries(test_json_parser flatccrt)
target_link_libraries(test_json_printer flatccrt)
target_link_libraries(test_json flatccrt)

add_test(test_basic_parse test_basic_parse${CMAKE_EXECUTABLE_SUFFIX})
add_test(test_json_parser test_json_parser${CMAKE_EXECUTABLE_SUFFIX})
add_test(test_json_printer test_json_printer${CMAKE_EXECUTABLE_SUFFIX})
add_test(test_json test_json${CMAKE_EXECUTABLE_SUFFIX})

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
    add_dependencies(${trg} gen_monster_test_json)
    add_test(${trg} ${trg}${CMAKE_EXECUTABLE_SUFFIX})
    set_target_properties(${trg} PROPERTIES COMPILE_FLAGS ${flags})
endmacro()

jstest(json_test_uql "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=1")
jstest(json_test_uql_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED_LIST=0")
jstest(json_test_uq "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=1")
jstest(json_test_uq_off "-DFLATCC_JSON_PARSE_ALLOW_UNQUOTED=0")
jstest(json_test "-DFLATCC_JSON_PARSE_WIDE_SPACE=1")
