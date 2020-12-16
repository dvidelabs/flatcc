include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/zzz_monster_test.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli -a --prefix=zzz_ --stdout "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/zzz_monster_test.h"
    DEPENDS flatcc::cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)
add_executable(monster_test_prefix monster_test_prefix.c "${GEN_DIR}/zzz_monster_test.h")
target_link_libraries(monster_test_prefix flatcc::runtime)

add_test(monster_test_prefix monster_test_prefix)
