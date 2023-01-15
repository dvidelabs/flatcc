include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/monster_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

file(GLOB FBS_FS ${FBS_DIR}/*.fbs)
add_custom_command (
    OUTPUT ${GEN_DIR}/monster_test.bfbs
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli --schema -o "${GEN_DIR}" "${FBS_DIR}/monster_test.fbs"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate reflection_test fbs headers"
)

add_executable(reflection_test reflection_test.c ${GEN_DIR}/monster_test.bfbs)
target_link_libraries(reflection_test flatccrt)

add_test(reflection_test reflection_test${CMAKE_EXECUTABLE_SUFFIX})
