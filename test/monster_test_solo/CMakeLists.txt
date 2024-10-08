include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

file(GLOB FBS_FS ${FBS_DIR}/*.fbs)
add_custom_command (
    OUTPUT ${GEN_DIR}/monster_test.h
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -cwv --reader --stdout "${FBS_DIR}/attributes.fbs" "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/monster_test.h"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate monster_test_solo fbs headers"
)

include_directories("${GEN_DIR}" "${INC_DIR}")
add_executable(monster_test_solo monster_test_solo.c ${GEN_DIR}/monster_test.h)
target_link_libraries(monster_test_solo flatccrt)

add_test(monster_test_solo monster_test_solo${CMAKE_EXECUTABLE_SUFFIX})
