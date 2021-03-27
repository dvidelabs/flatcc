set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/optional_scalars_test")

flatcc_generate_sources(
    NAME optional_scalars_test
    SCHEMA_FILES "${FBS_DIR}/optional_scalars_test.fbs"
    ALL
    JSON
    OUTPUT_DIR "${GEN_DIR}"
)

add_executable(optional_scalars_test optional_scalars_test.c)
target_link_libraries(optional_scalars_test PRIVATE flatcc::runtime flatcc_generated::optional_scalars_test)

add_test(NAME optional_scalars_test COMMAND optional_scalars_test)
