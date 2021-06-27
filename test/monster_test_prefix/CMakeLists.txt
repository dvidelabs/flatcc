set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(
    NAME monster_test_prefix
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL
    PREFIX zzz_
    OUTPUT_DIR "${GEN_DIR}"
    OUTFILE "zzz_monster_test.h"
)

add_executable(monster_test_prefix monster_test_prefix.c)
target_link_libraries(monster_test_prefix PRIVATE flatcc::runtime flatcc_generated::monster_test_prefix)

add_test(NAME monster_test_prefix COMMAND monster_test_prefix)
