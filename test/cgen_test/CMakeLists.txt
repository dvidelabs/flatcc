include(CTest)

include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_executable(cgen_test
    cgen_test.c
)

target_link_libraries(cgen_test
    flatcc
)

add_test(cgen_test cgen_test${CMAKE_EXECUTABLE_SUFFIX})


# Compilation of the generated code tests many import edge cases 
# in the parser and code generator but due to CMake limitations, 
# custom target dependencies only work for Make build targets.

add_custom_target(test_generated
    COMMAND ./cgen_test${CMAKE_EXECUTABLE_SUFFIX} > test_generated${CMAKE_EXECUTABLE_SUFFIX}.c
    COMMAND ${CMAKE_C_COMPILER} test_generated${CMAKE_EXECUTABLE_SUFFIX}.c -c -I${CMAKE_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/cgen_test
    )
add_dependencies(test_generated cgen_test)

if(${CMAKE_MAKE_PROGRAM} MATCHES make)
add_test(test_generated ${CMAKE_MAKE_PROGRAM} test_generated)
endif(${CMAKE_MAKE_PROGRAM} MATCHES make)
