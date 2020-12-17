set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL OUTPUT_DIR "${GEN_DIR}" DEFINITION_FILES monster.fbs)

add_executable(monster monster.c "${GEN_DIR}/monster_builder.h")
target_link_libraries(monster flatcc::runtime)

if (FLATCC_TEST)
    add_test(NAME monster COMMAND monster)
endif()
