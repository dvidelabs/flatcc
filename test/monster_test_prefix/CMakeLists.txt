include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

file(GLOB FBS_FS ${FBS_DIR}/*.fbs)
add_custom_command (
    OUTPUT ${GEN_DIR}/zzz_monster_test.h
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a --prefix=zzz_ --stdout "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/zzz_monster_test.h"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate monster_test_prefix fbs headers"
)


add_executable(monster_test_prefix monster_test_prefix.c ${GEN_DIR}/zzz_monster_test.h)
target_link_libraries(monster_test_prefix flatccrt)

add_test(monster_test_prefix monster_test_prefix${CMAKE_EXECUTABLE_SUFFIX})
