add_executable(cgen_test
    cgen_test.c
)

target_link_libraries(cgen_test
    flatcc::libflatcc
)

add_test(NAME cgen_test COMMAND cgen_test)


# Compilation of the generated code tests many import edge cases 
# in the parser and code generator but due to CMake limitations, 
# custom target dependencies only work for Make build targets.
#
# expansion of flags results in quotes the compiler won't eat,
# separating arguments should fix this, but not sure how portable it is.
# see also http://stackoverflow.com/questions/9870162/avoid-quoting-in-cmake-add-custom-command
separate_arguments(CUSTOM_C_FLAGS UNIX_COMMAND "${CMAKE_C_FLAGS}")

add_custom_target(test_generated
    COMMAND cgen_test > test_generated${CMAKE_EXECUTABLE_SUFFIX}.c
    COMMAND ${CMAKE_C_COMPILER} ${CUSTOM_C_FLAGS} test_generated${CMAKE_EXECUTABLE_SUFFIX}.c -c
        -I${CMAKE_SOURCE_DIR}/include
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/cgen_test
    BYPRODUCTS test_generated${CMAKE_EXECUTABLE_SUFFIX}.c
    DEPENDS cgen_test
    )

add_test(NAME test_generated COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}" --target test_generated --config $<CONFIG>)
