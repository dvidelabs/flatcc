include_directories (
    "${PROJECT_SOURCE_DIR}/include"
)

add_executable(test_cgen
    test_cgen.c
)

target_link_libraries(test_cgen
    PRIVATE flatcc
)
