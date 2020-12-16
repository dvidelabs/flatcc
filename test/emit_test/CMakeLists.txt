include(CTest)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/emit_test")

include_directories("${GEN_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/emit_test_builder.h" "${GEN_DIR}/emit_test_reader.h" "${GEN_DIR}/emit_test_verifier.h"
        "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc::cli -a -o "${GEN_DIR}" "${FBS_DIR}/emit_test.fbs"
    DEPENDS flatcc::cli "${FBS}"
)
add_executable(emit_test emit_test.c "${GEN_DIR}/emit_test_builder.h")
target_link_libraries(emit_test flatcc::runtime)

add_test(emit_test emit_test)
