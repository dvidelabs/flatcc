include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/optional_scalars_test")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/optional_scalars_test_builder.h" "${GEN_DIR}/optional_scalars_test_json_parser.h"
        "${GEN_DIR}/optional_scalars_test_json_printer.h" "${GEN_DIR}/optional_scalars_test_reader.h"
        "${GEN_DIR}/optional_scalars_test_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli -a --json -o "${GEN_DIR}" "${FBS_DIR}/optional_scalars_test.fbs"
    DEPENDS flatcc::cli "${FBS_DIR}/optional_scalars_test.fbs"
)
add_executable(optional_scalars_test optional_scalars_test.c "${GEN_DIR}/optional_scalars_test_builder.h"
    "${GEN_DIR}/optional_scalars_test_json_printer.h" "${GEN_DIR}/optional_scalars_test_json_parser.h")
target_link_libraries(optional_scalars_test flatcc::runtime)

add_test(optional_scalars_test optional_scalars_test)
