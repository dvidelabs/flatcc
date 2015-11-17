include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_library(flatccrt
    builder.c
    emitter.c
    verifier.c
)
