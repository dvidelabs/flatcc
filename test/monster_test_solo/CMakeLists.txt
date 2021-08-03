include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_monster_test_solo ALL) 
add_custom_command (
    TARGET gen_monster_test_solo
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -cwv --reader --stdout "${FBS_DIR}/attributes.fbs" "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/monster_test.fbs" > "${GEN_DIR}/monster_test.h" DEPENDS flatcc_cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs" "${FBS_DIR}/attributes.fbs"
)

include_directories("${GEN_DIR}" "${INC_DIR}")
add_executable(monster_test_solo monster_test_solo.c)
add_dependencies(monster_test_solo gen_monster_test_solo)
target_link_libraries(monster_test_solo flatccrt)

add_test(monster_test_solo monster_test_solo${CMAKE_EXECUTABLE_SUFFIX})
