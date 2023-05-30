include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

file(GLOB_RECURSE LIB_SOURCES
    "*.c"
)

add_library(flatccrt ${LIB_SOURCES}
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

wdk_add_library(flatccrtwdk ${LIB_SOURCES})

if (FLATCC_INSTALL)
    install(TARGETS flatccrt DESTINATION ${lib_dir})
    install(TARGETS flatccrtwdk DESTINATION ${lib_dir})
endif()
