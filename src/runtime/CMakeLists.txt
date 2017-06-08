include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_library(flatccrt
    builder.c
    emitter.c
    verifier.c
    json_parser.c
    json_printer.c
)

add_library(flatccrt_shared SHARED builder.c emitter.c verifier.c json_parser.c json_printer.c)

if (FLATCC_INSTALL)
    install(TARGETS flatccrt DESTINATION lib)
    install(TARGETS flatccrt_shared DESTINATION lib)
endif()
