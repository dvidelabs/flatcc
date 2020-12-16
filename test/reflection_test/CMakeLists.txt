include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}")

add_custom_target(gen_reflection_test ALL) 
add_custom_command (
    TARGET gen_reflection_test
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli --schema -o "${GEN_DIR}" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/monster_test.fbs" "${FBS_DIR}/include_test1.fbs" "${FBS_DIR}/include_test2.fbs"
)
add_executable(reflection_test reflection_test.c)
target_link_libraries(reflection_test flatcc::runtime)

add_test(reflection_test reflection_test)
