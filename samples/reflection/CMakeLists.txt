include(CTest)

#
# This projects depends headers generated from reflection.fbs but these
# are pre-generated in `include/flatcc/reflection` so we don't need to
# build them here.
#
# What we do build is a binary schema `monster.bfbs` for the monster
# sample, and the actual C source of this project.
#

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_monster_bfbs ALL)
add_custom_command (
    TARGET gen_monster_bfbs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli --schema -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster.fbs"
)
add_executable(bfbs2json bfbs2json.c)
add_dependencies(bfbs2json gen_monster_bfbs)
target_link_libraries(bfbs2json flatccrt)

if (FLATCC_TEST)
    add_test(bfbs2json bfbs2json${CMAKE_EXECUTABLE_SUFFIX} ${GEN_DIR}/monster.bfbs)
endif()
