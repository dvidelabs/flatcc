include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/monster_test.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    # We could also use the recursive -r option, but this tests adding files manually to the output file.
    COMMAND flatcc::cli -cwv --reader -o "${GEN_DIR}" "--outfile=monster_test.h" "${FBS_DIR}/attributes.fbs"
        "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs"
        "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/attributes.fbs"
)
add_executable(monster_test_concat monster_test_concat.c "${GEN_DIR}/monster_test.h")
target_link_libraries(monster_test_concat flatcc::runtime)

add_test(monster_test_concat monster_test_concat)
