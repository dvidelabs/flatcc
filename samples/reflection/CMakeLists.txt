include(CTest)

#
# This projects depends headers generated from reflection.fbs but these
# are pre-generated in `include/flatcc/reflection` so we don't need to
# build them here.
#
# What we do build is a binary schema `monster.bfbs` for the monster
# sample, and the actual C source of this project.
#

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/samples/monster")

include_directories("${GEN_DIR}" "${INC_DIR}")

file(GLOB FBS_FS ${FBS_DIR}/*.fbs)
list(TRANSFORM FBS_FS
    REPLACE "${FBS_DIR}/(.*)\\.fbs$" "${GEN_DIR}/\\1.bfbs"
    OUTPUT_VARIABLE FBS_BFBS
)

add_custom_command (
    OUTPUT ${FBS_BFBS}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli --schema -o "${GEN_DIR}" "${FBS_DIR}/monster.fbs"
    DEPENDS flatcc_cli "${FBS_FS}"
    COMMENT "Generate bfbs2json fbs headers"
)

add_executable(bfbs2json bfbs2json.c ${FBS_BFBS})
target_link_libraries(bfbs2json flatccrt)

if (FLATCC_TEST)
    add_test(bfbs2json bfbs2json${CMAKE_EXECUTABLE_SUFFIX} ${GEN_DIR}/monster.bfbs)
endif()
