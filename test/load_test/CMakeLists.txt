include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_command(
    OUTPUT "${GEN_DIR}/attributes_builder.h" "${GEN_DIR}/attributes_reader.h" "${GEN_DIR}/attributes_verifier.h"
        "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/include_test1_builder.h" "${GEN_DIR}/include_test1_reader.h" "${GEN_DIR}/include_test1_verifier.h"
        "${GEN_DIR}/include_test2_builder.h" "${GEN_DIR}/include_test2_reader.h" "${GEN_DIR}/include_test2_verifier.h"
        "${GEN_DIR}/monster_test_builder.h" "${GEN_DIR}/monster_test_reader.h" "${GEN_DIR}/monster_test_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)
add_executable(load_test load_test.c "${GEN_DIR}/monster_test_builder.h")
target_link_libraries(load_test flatccrt)

add_test(load_test load_test${CMAKE_EXECUTABLE_SUFFIX})

