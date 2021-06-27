set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/emit_test")

flatcc_generate_sources(
    NAME emit_test
    SCHEMA_FILES "${FBS_DIR}/emit_test.fbs"
    ALL
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(emit_test emit_test.c)
target_link_libraries(emit_test PRIVATE flatcc::runtime flatcc_generated::emit_test)

add_test(NAME emit_test COMMAND emit_test)
