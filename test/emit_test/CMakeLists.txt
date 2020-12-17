set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/emit_test")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL OUTPUT_DIR "${GEN_DIR}" DEFINITION_FILES "${FBS_DIR}/emit_test.fbs")

add_executable(emit_test emit_test.c "${GEN_DIR}/emit_test_builder.h")
target_link_libraries(emit_test flatcc::runtime)

add_test(NAME emit_test COMMAND emit_test)
