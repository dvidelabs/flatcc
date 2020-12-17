set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

# TODO: use recursive
flatcc_generate_sources(ALL OUTPUT_DIR "${GEN_DIR}"
    DEFINITION_FILES "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs")

add_executable(flatc_compat flatc_compat.c "${GEN_DIR}/monster_test_reader.h" "${GEN_DIR}/monster_test_verifier.h")
target_link_libraries(flatc_compat flatcc::runtime)

add_custom_command (
    TARGET flatc_compat POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/monsterdata_test.mon" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_test(NAME flatc_compat COMMAND flatc_compat)
