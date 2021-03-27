set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(
    NAME load_test
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(load_test load_test.c)
target_link_libraries(load_test PRIVATE flatcc::runtime flatcc_generated::load_test)

add_test(NAME load_test COMMAND load_test)

