set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/optional_scalars_test")

include_directories("${GEN_DIR}")

flatcc_generate_sources(ALL JSON OUTPUT_DIR "${GEN_DIR}"
    DEFINITION_FILES "${FBS_DIR}/optional_scalars_test.fbs"
)

add_executable(optional_scalars_test optional_scalars_test.c "${GEN_DIR}/optional_scalars_test_builder.h"
    "${GEN_DIR}/optional_scalars_test_json_printer.h" "${GEN_DIR}/optional_scalars_test_json_parser.h")
target_link_libraries(optional_scalars_test flatcc::runtime)

add_test(NAME optional_scalars_test COMMAND optional_scalars_test)
