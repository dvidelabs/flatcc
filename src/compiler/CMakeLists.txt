include_directories (
    "${PROJECT_SOURCE_DIR}/external"
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/config"
)

add_library(flatcc
    ${PROJECT_SOURCE_DIR}/external/hash/cmetrohash64.c
    ${PROJECT_SOURCE_DIR}/external/hash/str_set.c
    hash_tables/symbol_table.c
    hash_tables/scope_table.c
    hash_tables/name_table.c
    hash_tables/value_set.c
    fileio.c
    parser.c
    semantics.c
    coerce.c
    flatcc.c
    codegen_c_reader.c
    codegen_c_sort.c
    codegen_c_builder.c
    codegen_c_verifier.c
    codegen_schema.c
    # needed for building binary schema
    ../runtime/builder.c
    ../runtime/emitter.c
)
