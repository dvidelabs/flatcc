# Note: This re-uses the samples/monster fbs and .c file.

# We use our own separate gen dir so we don't clash with the real monster sample.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/monster_test_cpp")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL OUTPUT_DIR "${GEN_DIR}"
    DEFINITION_FILES "${FBS_DIR}/monster.fbs"
)

add_executable(monster_test_cpp monster_test.cpp "${GEN_DIR}/monster_builder.h")
target_link_libraries(monster_test_cpp flatcc::runtime)

add_test(NAME monster_test_cpp COMMAND monster_test_cpp)

