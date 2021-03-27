set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(
    NAME flatc_compat
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(flatc_compat flatc_compat.c)
target_link_libraries(flatc_compat PRIVATE flatcc::runtime flatcc_generated::flatc_compat)

add_custom_command (
    TARGET flatc_compat POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/monsterdata_test.mon" "${CMAKE_CURRENT_BINARY_DIR}"
)

add_test(NAME flatc_compat COMMAND flatc_compat)
