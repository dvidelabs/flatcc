add_executable(flatcc_cli
    flatcc_cli.c
)
add_executable(flatcc::cli ALIAS flatcc_cli)

target_link_libraries(flatcc_cli PRIVATE flatcc)
target_include_directories(flatcc_cli PRIVATE "${PROJECT_SOURCE_DIR}/config")

# Rename because the libflatcc library and the flatcc executable would
# conflict if they had the same target name `flatcc`.
set_target_properties(flatcc_cli PROPERTIES
    OUTPUT_NAME flatcc
    EXPORT_NAME cli
)
if(NOT (CMAKE_VERSION VERSION_LESS 3.4))
    # TODO: cmake versions <3.4 do not support generator expressions in OUTPUT_NAME
    set_target_properties(flatcc_cli PROPERTIES
        OUTPUT_NAME flatcc$<$<CONFIG:Debug>:_d>
    )
endif()

if (FLATCC_INSTALL)
    install(TARGETS flatcc_cli EXPORT flatcc_exports DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
