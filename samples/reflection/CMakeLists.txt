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

flatcc_generate_sources(
    NAME reflection_sample
    SCHEMA_FILES "${FBS_DIR}/monster.fbs"
    BINARY_SCHEMA
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(bfbs2json bfbs2json.c)
target_link_libraries(bfbs2json PRIVATE flatcc::runtime flatcc_generated::reflection_sample)

add_test(NAME bfbs2json COMMAND bfbs2json "${GEN_DIR}/monster.bfbs")
