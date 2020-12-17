set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(SCHEMA OUTPUT_DIR "${GEN_DIR}"
    DEFINITION_FILES "${FBS_DIR}/monster_test.fbs")

add_executable(reflection_test reflection_test.c "${GEN_DIR}/monster_test.bfbs")
target_link_libraries(reflection_test flatcc::runtime)

add_test(NAME reflection_test COMMAND reflection_test)
