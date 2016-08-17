# Ubuntu 14.04 (Trusty)
#cmake_minimum_required (VERSION 2.8.12.2)
# Centos 7
#cmake_minimum_required (VERSION 2.8.11)
cmake_minimum_required (VERSION 2.8)

project (FlatCC C)

#
# NOTE: when changing build options, clean the build using on of:
#
#   scripts/cleanall.sh
#   scripts/test.sh
#

# Force use of portable shims such as providing `static_assert`, and
# `stdaligh.h`. Otherwise this option is automatically enabled for some
# known compiler configurations below.
option (FLATCC_PORTABLE
    "include extra headers for compilers that do not support certain C11 features" OFF)

# Only build the runtime library - mostly intended in combination with
# FLATCC_INSTALL for cross compiling targets.
option(FLATCC_RTONLY "enable build of runtime library only" OFF)

# Use with or witout FLATCC_RTONLY to enable install targets.
# Libraries are built statically by default, but can CMake's
# cmake -DBUILD_SHARED_LIBS=on can override.
option(FLATCC_INSTALL "enable build of runtime library only" OFF)

# Disable build of tests and samples. Due to custom build step
# dependency on flatcc tool, some custom build configurations may
# experience issues, and this option can then help.
option(FLATCC_TEST "enable tests" ON)

# Affects the flatbuffer verify operation. Normally a verify should just
# quickly reject invalid buffers but for troubleshooting, assertions can
# enabled. This requires rebuilding the runtime library and will likely
# break test cases (those that tests that an invalid buffer is invalid).
option (FLATCC_DEBUG_VERIFY
    "assert on verify failure in runtime lib" OFF)

# Reflection is the compilers ability to generate binary schema output
# (.bfbs files). This requires using generated code from
# `reflection.fbs`. During development it may not be possible to
# compile with reflection enabled because it can become impossible to
# fix broken builds. It may also be disabled simple because it isn't
# needed.
option (FLATCC_REFLECTION
    "generation of binary flatbuffer schema files" ON)

# FLATCC_NATIVE_OPTIM and FLATCC_FAST_DOUBLE affects json parsing,
# especially if the content is pretty printed. But it is plenty
# fast without these settings in most cases. Not recommended.
option (FLATCC_NATIVE_OPTIM
    "use machine native optimizations like SSE 4.2" OFF)

# Fast grisu3 string/floating point conversion still depends on strtod
# for about 1-2% of the conversions in order to produce an exact result.
# By allowing a minor difference in the least significant bits, this
# dependeny can be avoided, and speed improved. Some strtod
# implementations call strlen which is really slow on large JSON
# buffers, and catastrophic on buffers that are not zero-terminated -
# regardless of size. Most platforms have a decent strtod these days.
option (FLATCC_FAST_DOUBLE
    "faster but slightly incorrect floating point parser (json)" OFF)

# -Werror is only set for some compiler versions that are believed to
# to not generate any warnings. If the assumption breaks, disable
# this option if the warning is not significant.
option (FLATCC_ALLOW_WERROR "allow -Werror to be configured" ON)

# Experimental setting - sometimes the code branches on a constant
# expression in order to select the best option for a given type size or
# similar. Sometimes compilers don't like that. If this issue surfaces,
# try using this option.
option (FLATCC_IGNORE_CONST_COND "silence const condition warnings" OFF)

if (FLATCC_RTONLY)
    set(FLATCC_TEST off)
endif()

if (FLATCC_TEST)
    enable_testing()
endif()

if (FLATCC_DEBUG_VERIFY)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLATCC_DEBUG_VERIFY=1")
endif()

if (FLATCC_REFLECTION)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLATCC_REFLECTION=1")
    file(WRITE ${PROJECT_SOURCE_DIR}/build/reflection_enabled "REFLECTION=1")
    file(REMOVE ${PROJECT_SOURCE_DIR}/build/reflection_disabled)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLATCC_REFLECTION=0")
    file(REMOVE ${PROJECT_SOURCE_DIR}/build/reflection_enabled)
    file(WRITE ${PROJECT_SOURCE_DIR}/build/reflection_disabled "REFLECTION=0")
endif()

if (FLATCC_NATIVE_OPTIM)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native -DFLATCC_USE_SSE4_2=1")
endif()

if (FLATCC_FAST_DOUBLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGRISU3_PARSE_ALLOW_ERROR -DFLATCC_USE_GRISU3=1")
endif()


# The folder of this directory, as apposed to CMAKE_BINARY_DIR
# which would usually be the build/Release and build/Debug paths
set (dist_dir "${PROJECT_SOURCE_DIR}")
# set (dist_dir "${CMAKE_BINARY_DIR}")

# Note: for compiling generated C code, warnings of unused functions
# and constants should be turned off - those are plentiful. They are
# silenced for Clang, GCC and MSVC in generated headers.headers.

if (CMAKE_C_COMPILER_ID MATCHES "Clang")
    # Clang or AppleClang

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pedantic -Wall -Wextra")
    if (FLATCC_ALLOW_WERROR)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    endif()
    if (FLATCC_IGNORE_CONST_COND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-tautological-constant-out-of-range-compare")
    endif()

    # To get assembly output
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -save-temps")

elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
                    OUTPUT_VARIABLE GCC_VERSION)
    if (GCC_VERSION VERSION_LESS 4.7)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
        # We need stdalign.h
        set(FLATCC_PORTABLE true)
    else()
        # -std=c11 only available as of 4.7
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -pedantic -Wall -Wextra")
        if (FLATCC_ALLOW_WERROR)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
        endif()
    endif()
    if (FLATCC_IGNORE_CONST_COND)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-type-limits")
    endif()

    # In gcc 4.8 it is not possible to suppress this warning using
    # #pragma GCC diagnostic ignored "-Wunused-function"
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-type-limits")

elseif (CMAKE_C_COMPILER_ID STREQUAL "Intel")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall -Wextra")
elseif (MSVC) # using STREQUAL here conflicts with string interpretation changes in CMake
    # -DFLATCC_PORTABLE also required, but set earlier
    # -W3 is the highest warning level that is reasonable.
    # See include/flatcc/portable/pwarnings.h for disabled warnings.
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W3 -D_CRT_SECURE_NO_WARNINGS")
    # MSVC 2013 (1800) supports inline variable declations
    # while MSVC 2010 (1600) does not.
    if (MSVC_VERSION STRLESS "1800")
        # Disables monster sample build which uses C99 style variable decls.
        set (FLATCC_NEED_C89_VAR_DECLS true)
    endif()
    set(FLATCC_PORTABLE true)
else()
    # Best effort
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    set(FLATCC_PORTABLE true)
endif()

if (FLATCC_PORTABLE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFLATCC_PORTABLE")
endif()

if (CLANG_VERSION)
    message(STATUS "CLANG_VERSION: ${CLANG_VERSION}")
endif()
if (GCC_VERSION)
    message(STATUS "GCC_VERSION: ${GCC_VERSION}")
endif()
message(STATUS "Configured C_FLAGS: ${CMAKE_C_FLAGS}")

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_DEBUG_POSTFIX "_d")

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_EXECUTABLE_SUFFIX "_d${CMAKE_EXECUTABLE_SUFFIX}")
endif()


if (FLATCC_RTONLY)
    # The targets we copy to bin and lib directories, i.e. not tests.
    set(dist_targets
        flatccrt
    )
    add_subdirectory(src/runtime)
else()
    # The targets we copy to bin and lib directories, i.e. not tests.
    set(dist_targets
        flatcc
        flatccrt
        flatcc_cli
    )
    add_subdirectory(src/runtime)
    add_subdirectory(src/compiler)
    add_subdirectory(src/cli)
endif()

# disabled by FLATCC_RTONLY
if (FLATCC_TEST)
    add_subdirectory(test)
    add_subdirectory(samples)
endif()

set_target_properties(${dist_targets}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${dist_dir}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${dist_dir}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${dist_dir}/bin"
)

if (FLATCC_INSTALL)
    install(DIRECTORY include/flatcc DESTINATION include)
endif()

