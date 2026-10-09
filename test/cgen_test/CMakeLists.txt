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
#
# expansion of flags results in quotes the compiler won't eat,
# separating arguments should fix this, but not sure how portable it is.
# see also http://stackoverflow.com/questions/9870162/avoid-quoting-in-cmake-add-custom-command
separate_arguments(CUSTOM_C_FLAGS UNIX_COMMAND "${CMAKE_C_FLAGS}")

add_custom_target(test_generated 
    COMMAND ./cgen_test${CMAKE_EXECUTABLE_SUFFIX} > test_generated${CMAKE_EXECUTABLE_SUFFIX}.c
    COMMAND ${CMAKE_C_COMPILER} ${CUSTOM_C_FLAGS} test_generated${CMAKE_EXECUTABLE_SUFFIX}.c -c
        -I${CMAKE_SOURCE_DIR}/include WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/cgen_test
    )
add_dependencies(test_generated cgen_test)

# Might be related to:
# https://cmake.org/Bug/view.php?id=14963#c37230
# https://github.com/ninja-build/ninja/issues/760
if(${CMAKE_MAKE_PROGRAM} MATCHES make)
# this is now also broken for make - the system include path is not
# visible so build fails on <assert.h> not found in the custom build
# stage where CMAKE_C_COMPILER uses a compiler call that has this
# behavior
#add_test(test_generated ${CMAKE_MAKE_PROGRAM} test_generated)
endif(${CMAKE_MAKE_PROGRAM} MATCHES make)
