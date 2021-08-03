include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/optional_scalars_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_optional_scalars_test ALL) 
add_custom_command (
    TARGET gen_optional_scalars_test
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a --json -o "${GEN_DIR}" "${FBS_DIR}/optional_scalars_test.fbs"
)
add_executable(optional_scalars_test optional_scalars_test.c)
add_dependencies(optional_scalars_test gen_optional_scalars_test)
target_link_libraries(optional_scalars_test flatccrt)

add_test(optional_scalars_test optional_scalars_test${CMAKE_EXECUTABLE_SUFFIX})
