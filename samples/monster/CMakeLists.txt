include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_command (
    OUTPUT "${GEN_DIR}/flatbuffers_common_builder.h" "${GEN_DIR}/flatbuffers_common_reader.h"
        "${GEN_DIR}/monster_builder.h" "${GEN_DIR}/monster_reader.h" "${GEN_DIR}/monster_verifier.h"
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/monster.fbs" 
)
add_executable(monster monster.c "${GEN_DIR}/monster_builder.h")
target_link_libraries(monster flatccrt)

if (FLATCC_TEST)
    add_test(monster monster${CMAKE_EXECUTABLE_SUFFIX})
endif()
