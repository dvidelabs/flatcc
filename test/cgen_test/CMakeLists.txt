include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_executable(cgen_test
    cgen_test.c
)

target_link_libraries(cgen_test
    flatcc
)
