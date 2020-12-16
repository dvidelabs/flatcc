include(CTest)

#
# This projects depends headers generated from reflection.fbs but these
# are pre-generated in `include/flatcc/reflection` so we don't need to
# build them here.
#
# What we do build is a binary schema `monster.bfbs` for the monster
# sample, and the actual C source of this project.
#

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/monster.bfbs"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli --schema -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/monster.fbs"
)
add_custom_target(build_moster_bfbs ALL
    DEPENDS "${GEN_DIR}/monster.bfbs"
)

add_executable(bfbs2json bfbs2json.c)
target_link_libraries(bfbs2json flatcc::runtime)

if (FLATCC_TEST)
    add_test(NAME bfbs2json COMMAND bfbs2json "${GEN_DIR}/monster.bfbs")
endif()
