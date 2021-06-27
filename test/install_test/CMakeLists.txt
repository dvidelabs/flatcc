# This file tests the installation of flatbuffers. It is not meant to be used as a subfolder of test.

cmake_minimum_required(VERSION 3.3)
project(flatcc_install_test)

# Fake the base of the project to be the root of flatcc
set(CMAKE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

enable_testing()

find_package(flatcc REQUIRED)

if(NOT DEFINED FLATCC_RTONLY)
    message(FATAL_ERROR "Variable FLATCC_RTONLY not defined")
endif()

if(NOT TARGET flatcc::runtime)
    message(FATAL_ERROR "flatcc::runtime target not available")
endif()

if(NOT TARGET flatcc::cli_native)
    message(FATAL_ERROR "flatcc::cli_native target not available")
endif()

if(NOT FLATCC_RTONLY)
    if(NOT TARGET flatcc::cli)
        message(FATAL_ERROR "flatcc::cli target not available")
    endif()

    if(NOT TARGET flatcc::compiler)
        message(FATAL_ERROR "flatcc::compiler target not available")
    endif()
endif()

include(FlatccGenerateSources)

# re-use flatcc monster_test and reflection_test

add_subdirectory("${PROJECT_SOURCE_DIR}/test/monster_test" monster_test)
add_subdirectory("${PROJECT_SOURCE_DIR}/test/reflection_test" reflection_test)
