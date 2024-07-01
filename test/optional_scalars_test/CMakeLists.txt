include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/optional_scalars_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

file(GLOB FBS_FS ${FBS_DIR}/*.fbs)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1_builder.h"
    OUTPUT_VARIABLE FBS_B_HS
)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1_reader.h"
    OUTPUT_VARIABLE FBS_R_HS
)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1_verifier.h"
    OUTPUT_VARIABLE FBS_V_HS
)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1_json_printer.h"
    OUTPUT_VARIABLE FBS_JPR_HS
)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1_json_parser.h"
    OUTPUT_VARIABLE FBS_JPA_HS
)

list(APPEND FBS_R_HS ${GEN_DIR}/flatbuffers_common_reader.h)
list(APPEND FBS_B_HS ${GEN_DIR}/flatbuffers_common_builder.h)

add_custom_command (
    OUTPUT ${FBS_B_HS} ${FBS_R_HS} ${FBS_V_HS} ${FBS_JPR_HS} ${FBS_JPA_HS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a --json -o "${GEN_DIR}" "${FBS_DIR}/optional_scalars_test.fbs"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate optional_scalars_test fbs headers"
)

add_executable(optional_scalars_test optional_scalars_test.c
    ${FBS_B_HS} ${FBS_R_HS} ${FBS_V_HS} ${FBS_JPR_HS} ${FBS_JPA_HS})

target_link_libraries(optional_scalars_test flatccrt)

add_test(optional_scalars_test optional_scalars_test${CMAKE_EXECUTABLE_SUFFIX})
