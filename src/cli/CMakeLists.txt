include_directories (
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/config"
)

add_executable(flatcc_cli
    flatcc_cli.c
)
add_executable(flatcc::cli ALIAS flatcc_cli)

target_link_libraries(flatcc_cli
    flatcc
)

# Rename because the libflatcc library and the flatcc executable would
# conflict if they had the same target name `flatcc`.
set_target_properties(flatcc_cli PROPERTIES
    OUTPUT_NAME flatcc
    EXPORT_NAME cli)

if (FLATCC_INSTALL)
    install(TARGETS flatcc_cli EXPORT flatcc_exports DESTINATION bin)
endif()
