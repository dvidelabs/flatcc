include(CTest)

# Note: This re-uses the samples/monster fbs and .c file.

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
# We use our own separate gen dir so we don't clash with the real monster sample.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/monster_test_cpp")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_command(
    OUTPUT "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/monster_builder.h" "${GEN_DIR}/monster_reader.h" "${GEN_DIR}/monster_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster.fbs"
)

add_executable(monster_test_cpp monster_test.cpp "${GEN_DIR}/monster_builder.h")
target_link_libraries(monster_test_cpp flatccrt)

add_test(monster_test_cpp monster_test_cpp${CMAKE_EXECUTABLE_SUFFIX})

