include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_monster_fbs ALL) 
add_custom_command (
    TARGET gen_monster_fbs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster.fbs" 
)
add_executable(monster monster.c)
add_dependencies(monster gen_monster_fbs)
target_link_libraries(monster flatccrt)

if (FLATCC_TEST)
    add_test(monster monster${CMAKE_EXECUTABLE_SUFFIX})
endif()
