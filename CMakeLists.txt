#cmake_minimum_required (VERSION 3.0.2)
cmake_minimum_required (VERSION 2.8.0)

project (FlatCC)

# The folder of this directory, as apposed to CMAKE_BINARY_DIR
# which would usually be the build/Release and build/Debug paths
set (dist_dir "${PROJECT_SOURCE_DIR}")
# set (dist_dir "${CMAKE_BINARY_DIR}")

# The targets we copy to bin and lib directories, i.e. not tests.
set(dist_targets
    flatcc
    flatccrt
    flatcc_cli
)

# Note: for compiling generated C code, warnings of unused functions
# and constants should be turned off - those are plentiful. They are
# silenced for Clang automaitically.

# -DFLATCC_PORTABLE can help with stdint.h and implementing endian
# swapping in generated code. Most most recent systems should compile
# without C11 and without -DFLATCC_PORTABLE

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -DFLATCC_PORTABLE")

# Note:GNUC -pendantic without -std=c11 yields many warnings we don't care about.
# Clang is fine.
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -pedantic -Wextra")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_DEBUG_POSTFIX "_d")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_EXECUTABLE_SUFFIX "_d${CMAKE_EXECUTABLE_SUFFIX}")
endif()

# Note: test cases (except test_cgen) are not built here. They run by
# bash script each time the test fires and triggers thsi build as well.
# These scripts respect the CC env. variable.

add_subdirectory(test)
add_subdirectory(src/compiler)
add_subdirectory(src/runtime)
add_subdirectory(src/cli)

set_target_properties(${dist_targets}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${dist_dir}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${dist_dir}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${dist_dir}/bin"
)
