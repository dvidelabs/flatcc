# Use the following function to generate C source files from flatbuffer definition files:
#
# flatcc_generate_sources(OUTPUT_DIR <directory where to write source files>
#                         BUILDER
#                         VERIFIER
#                         DEFINITION_FILES <list of flatbuffer definition files (.fbs)>
# )
#
# BUILDER and VERIFIER are boolean options. When specified they will instruct
# flatcc to generate builder / verifier source code.
#
# With cross-compiling you should provide the directory where the flatcc compiler executable is located
# in environment variable FLATCC_BUILD_BIN_PATH. If you use Conan and add flatcc as a build requirement
# this will be done automatically.

include(CMakeParseArguments)


function(flatcc_generate_sources)
    # parse function arguments
    set(output_options SCHEMA COMMON COMMON_READER COMMON_BUILDER BUILDER READER VERIFIER JSON_PARSER JSON_PRINTER JSON)
    # TODO: missing RECURSIVE option
    set(NO_VAL_ARGS ALL ${output_options})
    set(SINGLE_VAL_ARGS OUTPUT_DIR OUTFILE PREFIX TARGET)
    set(MULTI_VAL_ARGS DEFINITION_FILES COMPILE_FLAGS PATHS)

    cmake_parse_arguments(FLATCC "${NO_VAL_ARGS}" "${SINGLE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})

    if (NOT FLATCC_DEFINITION_FILES)
        message(FATAL_ERROR "No flatbuffer definition files provided")
    endif()

    if (NOT FLATCC_OUTPUT_DIR)
        set(FLATCC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    list(APPEND FLATCC_COMPILE_FLAGS -o "${FLATCC_OUTPUT_DIR}")

    foreach(path ${FLATCC_PATHS})
        list(APPEND FLATCC_COMPILE_FLAGS -I "${path}")
    endforeach()

    # if no option is passed, the default READER is used
    set(default_option_reader ON)
    foreach(output_option ${output_options})
        if(output_option)
            set(default_option_reader OFF)
        endif()
    endforeach()
    if(default_option_reader)
        set(FLATCC_READER ON)
    endif()

    # Add flatcc options

    if(FLATCC_PREFIX)
        list(APPEND FLATCC_COMPILE_FLAGS "--prefix=${FLATCC_PREFIX}")
    endif()

    # Handle reader option first as other option encompass reader
    if (FLATCC_READER)
        list(APPEND FLATCC_COMPILE_FLAGS --reader)
    endif()
    if (FLATCC_COMMON_READER)
        list(APPEND FLATCC_COMPILE_FLAGS --common)
    endif()
    if (FLATCC_COMMON_BUILDER)
        list(APPEND FLATCC_COMPILE_FLAGS --common)
    endif()
    if (FLATCC_BUILDER)
        list(APPEND FLATCC_COMPILE_FLAGS --builder)
        # Builder also generates reader
        set(FLATCC_READER ON)
    endif()
    if (FLATCC_VERIFIER)
        list(APPEND FLATCC_COMPILE_FLAGS --verifier)
        # verifier also generates reader
        set(FLATCC_READER ON)
    endif()
    if (FLATCC_JSON_PARSER)
        list(APPEND FLATCC_COMPILE_FLAGS --json-parser)
    endif()
    if (FLATCC_JSON_PRINTER)
        list(APPEND FLATCC_COMPILE_FLAGS --json-printer)
    endif()
    if (FLATCC_SCHEMA)
        list(APPEND FLATCC_COMPILE_FLAGS --schema)
    endif()
    # TODO: missing recursive option
    # if (FLATCC_RECURSIVE)
    #     list(APPEND FLATCC_COMPILE_FLAGS --recursive)
    # endif()

    # handle 'all', 'common', 'json' last as they encompass other options
    if (FLATCC_COMMON)
        list(APPEND FLATCC_COMPILE_FLAGS --common)
        set(FLATCC_COMMON_READER ON)
        set(FLATCC_COMMON_BUILDER ON)
    endif()
    if (FLATCC_JSON)
        list(APPEND FLATCC_COMPILE_FLAGS --json)
        set(FLATCC_JSON_PARSER ON)
        set(FLATCC_JSON_PRINTER ON)
    endif()
    if (FLATCC_ALL)
        list(APPEND FLATCC_COMPILE_FLAGS -a)
        set(FLATCC_READER ON)
        set(FLATCC_COMMON_BUILDER ON)
        set(FLATCC_COMMON_READER ON)
        set(FLATCC_BUILDER ON)
        set(FLATCC_VERIFIER ON)
        # TODO: missing recursive option
        # set(FLATCC_RECURSIVE ON)
    endif()

    # Calculate suffixes of output files.

    set(GENERATED_FILE_SUFFIXES)
    if(FLATCC_READER)
        list(APPEND GENERATED_FILE_SUFFIXES _reader.h)
    endif()
    if(FLATCC_BUILDER)
        list(APPEND GENERATED_FILE_SUFFIXES _builder.h)
    endif()
    if(FLATCC_VERIFIER)
        list(APPEND GENERATED_FILE_SUFFIXES _verifier.h)
    endif()
    if(FLATCC_JSON_PARSER)
        list(APPEND GENERATED_FILE_SUFFIXES _json_parser.h)
    endif()
    if(FLATCC_JSON_PRINTER)
        list(APPEND GENERATED_FILE_SUFFIXES _json_printer.h)
    endif()
    if(FLATCC_SCHEMA)
        list(APPEND GENERATED_FILE_SUFFIXES .bfbs)
    endif()

    set(ABSOLUTE_DEFINITION_FILES)
    set(OUTPUT_FILES)
    if(FLATCC_OUTFILE)
        list(APPEND FLATCC_COMPILE_FLAGS "--outfile=${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
        list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
    else()
        foreach(definition_file ${FLATCC_DEFINITION_FILES})
            get_filename_component(def_name_we "${definition_file}" NAME_WLE)
            foreach(suffix ${GENERATED_FILE_SUFFIXES})
                list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/${def_name_we}${suffix}")
            endforeach()
        endforeach()

        if(FLATCC_COMMON_READER)
            list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/flatbuffers_common_reader.h")
        endif()
        if(FLATCC_COMMON_BUILDER)
            list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/flatbuffers_common_builder.h")
        endif()
    endif()
    foreach(definition_file ${FLATCC_DEFINITION_FILES})
        file(REAL_PATH "${definition_file}" absolute_def_file)
        list(APPEND ABSOLUTE_DEFINITION_FILES "${absolute_def_file}")
    endforeach()

    message(VERBOSE "Flatcc output directory: ${FLATCC_OUTPUT_DIR}")
    message(VERBOSE "Flatcc output files: ${OUTPUT_FILES}")
    message(VERBOSE "Flatcc execute: flatcc;${FLATCC_COMPILE_FLAGS};${ABSOLUTE_DEFINITION_FILES}")
    add_custom_command(OUTPUT ${OUTPUT_FILES}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${FLATCC_OUTPUT_DIR}"
        COMMAND flatcc::cli ${FLATCC_COMPILE_FLAGS} ${ABSOLUTE_DEFINITION_FILES}
        DEPENDS flatcc::cli ${FLATCC_DEFINITION_FILES}
    )
    if(FLATCC_TARGET)
        # FIXME: possible improvement if cmake_minimum_required >= 3.1:
        #        use add_library(... INTERFACE) +  add_custom_command + target_sources(INTERFACE) + target_link_libraries()
        add_custom_target("${FLATCC_TARGET}" DEPENDS ${OUTPUT_FILES})
    endif()

endfunction()
