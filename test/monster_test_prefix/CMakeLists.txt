set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL PREFIX zzz_
    OUTPUT_DIR "${GEN_DIR}"
    OUTFILE "zzz_monster_test.h"
    DEFINITION_FILES "${FBS_DIR}/monster_test.fbs")

add_executable(monster_test_prefix monster_test_prefix.c "${GEN_DIR}/zzz_monster_test.h")
target_link_libraries(monster_test_prefix flatcc::runtime)

add_test(NAME monster_test_prefix COMMAND monster_test_prefix)
