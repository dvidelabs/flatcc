set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL OUTPUT_DIR "${GEN_DIR}"
    DEFINITION_FILES "${FBS_DIR}/monster_test.fbs")

add_executable(load_test load_test.c "${GEN_DIR}/monster_test_builder.h")
target_link_libraries(load_test flatcc::runtime)

add_test(NAME load_test COMMAND load_test)

