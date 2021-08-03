include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/emit_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_emit_test ALL) 
add_custom_command (
    TARGET gen_emit_test
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/emit_test.fbs"
    DEPENDS flatcc_cli "${FBS}"
)
add_executable(emit_test emit_test.c)
add_dependencies(emit_test gen_emit_test)
target_link_libraries(emit_test flatccrt)

add_test(emit_test emit_test${CMAKE_EXECUTABLE_SUFFIX})
