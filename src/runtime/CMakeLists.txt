add_library(flatccrt
    builder.c
    emitter.c
    refmap.c
    verifier.c
    json_parser.c
    json_printer.c
)
add_library(flatcc::runtime ALIAS flatccrt)

target_include_directories(flatccrt PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>)

set_target_properties(flatccrt PROPERTIES
    EXPORT_NAME runtime)

if (FLATCC_INSTALL)
    install(TARGETS flatccrt EXPORT flatcc_exports
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
endif()
