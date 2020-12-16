include(CTest)

# Note: This re-uses the samples/monster fbs and .c file.

# We use our own separate gen dir so we don't clash with the real monster sample.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/monster_test_cpp")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}")

add_custom_command(
    OUTPUT "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/monster_builder.h" "${GEN_DIR}/monster_reader.h" "${GEN_DIR}/monster_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/monster.fbs"
)

add_executable(monster_test_cpp monster_test.cpp "${GEN_DIR}/monster_builder.h")
target_link_libraries(monster_test_cpp flatcc::runtime)

add_test(NAME monster_test_cpp COMMAND monster_test_cpp)

