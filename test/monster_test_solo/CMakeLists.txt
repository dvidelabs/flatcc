set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/monster_test.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli -cwv --reader --stdout "${FBS_DIR}/attributes.fbs" "${FBS_DIR}/include_test2.fbs"
        "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/monster_test.h"
    DEPENDS flatcc::cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs"
        "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/attributes.fbs"
)

add_executable(monster_test_solo monster_test_solo.c "${GEN_DIR}/monster_test.h")
target_link_libraries(monster_test_solo flatcc::runtime)

add_test(NAME monster_test_solo COMMAND monster_test_solo)
