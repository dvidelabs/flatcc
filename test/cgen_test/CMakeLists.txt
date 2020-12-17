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

add_custom_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/test_generated${CMAKE_EXECUTABLE_SUFFIX}.c"
    COMMAND cgen_test > "${CMAKE_CURRENT_BINARY_DIR}/test_generated${CMAKE_EXECUTABLE_SUFFIX}.c"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/cgen_test
    DEPENDS cgen_test
)

if(NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error=unused-const-variable")
endif()
add_library(test_generated_objects STATIC EXCLUDE_FROM_ALL
    "${CMAKE_CURRENT_BINARY_DIR}/test_generated${CMAKE_EXECUTABLE_SUFFIX}.c"
)
target_include_directories(test_generated_objects PRIVATE "${PROJECT_SOURCE_DIR}/include")

add_custom_target(test_generated
    COMMAND "${CMAKE_COMMAND}" -E remove -f "${CMAKE_CURRENT_BINARY_DIR}/test_generated${CMAKE_EXECUTABLE_SUFFIX}.c" $<TARGET_OBJECTS:test_generated_objects>
    COMMAND "${CMAKE_COMMAND}" --build "${PROJECT_BINARY_DIR}" --target test_generated_objects --config $<CONFIG> --verbose
)

add_test(NAME test_generated COMMAND ${CMAKE_COMMAND} --build "${PROJECT_BINARY_DIR}" --target test_generated --config $<CONFIG>)
