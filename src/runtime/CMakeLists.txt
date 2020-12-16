include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_library(flatccrt
    builder.c
    emitter.c
    refmap.c
    verifier.c
    json_parser.c
    json_printer.c
)
add_library(flatcc::runtime ALIAS flatccrt)

target_include_directories(flatccrt PUBLIC $<INSTALL_INTERFACE:include>)

set_target_properties(flatccrt PROPERTIES
    EXPORT_NAME runtime)

if (FLATCC_INSTALL)
    install(TARGETS flatccrt EXPORT flatcc_exports DESTINATION ${lib_dir})
endif()
