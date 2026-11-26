set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GEN_HDR "${GEN_DIR}/clone_sample_builder.h")

add_custom_command(
    OUTPUT ${GEN_HDR}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/clone_sample.fbs"
    DEPENDS flatcc_cli "${CMAKE_CURRENT_SOURCE_DIR}/clone_sample.fbs"
    COMMENT "Generating FlatCC output from clone_sample.fbs"
    VERBATIM
)

add_custom_target(gen_clone_sample_fbs DEPENDS ${GEN_HDR})
add_executable(clone_sample clone_sample.c)
target_include_directories(clone_sample PRIVATE "${GEN_DIR}" "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(clone_sample flatccrt)
add_dependencies(clone_sample gen_clone_sample_fbs)
add_test(NAME clone_sample COMMAND clone_sample)
