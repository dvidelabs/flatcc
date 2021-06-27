if(TARGET flatcc::cli_native)
    message(WARNING "flatcc:::cli_native is already defined")
else()
    set(__find_program_flatcc OFF)
    if(CMAKE_CROSSCOMPILING)
        set(__find_program_flatcc ON)
    else()
        if(NOT TARGET flatcc::cli AND NOT DEFINED flatcc_FOUND)
            find_package(flatcc REQUIRED)
        endif()
        if(TARGET flatcc::cli)
            get_property(_flatcc_cli_alias TARGET flatcc::cli PROPERTY ALIASED_TARGET)
            if(_flatcc_cli_alias)
                add_executable(flatcc::cli_native ALIAS ${_flatcc_cli_alias})
            else()
                add_executable(flatcc::cli_native ALIAS flatcc::cli)
            endif()
        else()
            set(__find_program_flatcc ON)
        endif()
    endif()

    if(__find_program_flatcc)
        find_program(FLATCC_BIN
            NAMES flatcc flatcc_d
            PATHS "${FLATCC_ROOT_FOR_BUILD}" ENV FLATCC_ROOT_FOR_BUILD
            PATH_SUFFIXES bin
            DOC "Path of native flatcc executable"
            NO_DEFAULT_PATH
        )
        if(NOT FLATCC_BIN)
            message(FATAL_ERROR "Could not find native flatcc executable.")
        endif()
        add_executable(flatcc::cli_native IMPORTED)
        set_property(TARGET flatcc::cli_native PROPERTY IMPORTED_LOCATION "${FLATCC_BIN}")
    endif()
endif()

set(FlatCCNative_FOUND ON)
