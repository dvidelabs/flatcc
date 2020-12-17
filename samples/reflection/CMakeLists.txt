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

flatcc_generate_sources(SCHEMA OUTPUT_DIR "${GEN_DIR}" DEFINITION_FILES "${FBS_DIR}/monster.fbs")

add_executable(bfbs2json bfbs2json.c "${GEN_DIR}/monster.bfbs")
target_link_libraries(bfbs2json flatcc::runtime)

if (FLATCC_TEST)
    add_test(NAME bfbs2json COMMAND bfbs2json "${GEN_DIR}/monster.bfbs")
endif()
