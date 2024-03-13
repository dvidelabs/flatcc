include(CTest)

set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FBS_DIR "${PROJECT_SOURCE_DIR}/test/doublevec_test")

include_directories("${GEN_DIR}" "${INC_DIR}")

add_custom_target(gen_doublevec_test ALL) 
add_custom_command (
    TARGET gen_doublevec_test
    COMMAND cmake -E make_directory "${GEN_DIR}"
    COMMAND flatcc_cli -a -o "${GEN_DIR}" "${FBS_DIR}/doublevec_test.fbs"
    DEPENDS flatcc_cli "${FBS_DIR}/header.fbs"
)
add_executable(doublevec_test doublevec_test.c)
add_dependencies(doublevec_test gen_doublevec_test)
target_link_libraries(doublevec_test flatccrt)

add_test(doublevec_test doublevec_test${CMAKE_EXECUTABLE_SUFFIX})
