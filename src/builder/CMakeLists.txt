include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_library(flatccbuilder
    builder.c
    emitter.c
)
