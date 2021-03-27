set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(
    NAME monster_test
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(monster_test monster_test.c)
target_link_libraries(monster_test PRIVATE flatcc::runtime flatcc_generated::monster_test)

add_test(NAME monster_test COMMAND monster_test)
