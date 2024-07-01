include(CTest)

# Note: This re-uses the samples/monster fbs and .c file.

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
# We use our own separate gen dir so we don't clash with the real monster sample.
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/monster_test_cpp")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

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

list(APPEND FBS_R_HS ${GEN_DIR}/flatbuffers_common_reader.h)
list(APPEND FBS_B_HS ${GEN_DIR}/flatbuffers_common_builder.h)

add_custom_command (
    OUTPUT ${FBS_B_HS} ${FBS_R_HS} ${FBS_V_HS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate monster_test_cpp fbs headers"
)

add_executable(monster_test_cpp monster_test.cpp ${FBS_B_HS} ${FBS_R_HS} ${FBS_V_HS})
target_link_libraries(monster_test_cpp flatccrt)

add_test(monster_test_cpp monster_test_cpp${CMAKE_EXECUTABLE_SUFFIX})
