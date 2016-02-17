include_directories (
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/external"
)

add_library(flatccrt
    builder.c
    emitter.c
    verifier.c
    json_parser.c
    json_printer.c
)
