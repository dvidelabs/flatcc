include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_monster_test_prefix ALL) 
add_custom_command (
    TARGET gen_monster_test_prefix
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a --prefix=zzz_ --stdout "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/zzz_monster_test.h"
    DEPENDS flatcc_cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)
add_executable(monster_test_prefix monster_test_prefix.c)
add_dependencies(monster_test_prefix gen_monster_test_prefix)
target_link_libraries(monster_test_prefix flatccrt)

add_test(monster_test_prefix monster_test_prefix${CMAKE_EXECUTABLE_SUFFIX})
