set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

flatcc_generate_sources(
    NAME monster_test_concat
    SCHEMA_FILES "${FBS_DIR}/monster_test.fbs"
    ALL
    OUTPUT_DIR "${GEN_DIR}"
    OUTFILE "monster_test.h"
)

add_executable(monster_test_concat monster_test_concat.c)
target_link_libraries(monster_test_concat PRIVATE flatcc::runtime flatcc_generated::monster_test_concat)

add_test(NAME monster_test_concat COMMAND monster_test_concat)
