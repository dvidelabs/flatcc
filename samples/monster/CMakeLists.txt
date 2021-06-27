set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Don't use FBS_DIR to test whether flatcc_generate_sources knows to use the correct file.

flatcc_generate_sources(
    NAME monster_sample
    SCHEMA_FILES monster.fbs
    ALL
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(monster monster.c)
target_link_libraries(monster PRIVATE flatcc::runtime flatcc_generated::monster_sample)

add_test(NAME monster COMMAND monster)
