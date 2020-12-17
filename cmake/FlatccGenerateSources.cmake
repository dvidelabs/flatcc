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
    set(NO_VAL_ARGS ALL RECURSIVE ${output_options})
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

    # Add current source directory for finding dependencies
    set(absolute_flatcc_paths "${CMAKE_CURRENT_SOURCE_DIR}")
    # Also add directory of definition files
    foreach(definition_file FLATCC_DEFINITION_FILES)
    endforeach()

    set(ABSOLUTE_DEFINITION_FILES)
    foreach(definition_file ${FLATCC_DEFINITION_FILES})
        # TODO: file(REAL_PATH) if cmake_minimum_required_version >= 3.19
        if(IS_ABSOLUTE "${definition_file}")
            set(absolute_def_file "${definition_file}")
        else()
            set(absolute_def_file "${CMAKE_CURRENT_SOURCE_DIR}/${definition_file}")
        endif()
        list(APPEND ABSOLUTE_DEFINITION_FILES "${absolute_def_file}")

        get_filename_component(absolute_def_directory "${absolute_def_file}" DIRECTORY)
        list(APPEND absolute_flatcc_paths "${absolute_def_directory}")
    endforeach()

    foreach(path ${FLATCC_PATHS})
        # TODO: file(REAL_PATH) if cmake_minimum_required_version >= 3.19
        if(NOT IS_ABSOLUTE "${path}")
            set(path "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
        endif()
        list(APPEND absolute_flatcc_paths "${path}")
        list(APPEND FLATCC_COMPILE_FLAGS -I "${path}")
    endforeach()
    list(REMOVE_DUPLICATES absolute_flatcc_paths)

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
    if (FLATCC_RECURSIVE)
        list(APPEND FLATCC_COMPILE_FLAGS --recursive)
    endif()

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

    # grep each definition file recursively for includes, convert them to absolute paths and add them to a list
    set(ABSOLUTE_DEFINITIONS_DEPENDENCIES)
    set(absolute_definition_files_todo ${ABSOLUTE_DEFINITION_FILES})
    while(absolute_definition_files_todo)
        list(GET absolute_definition_files_todo 0 current_deffile)
        # TODO: use if(absolute_definition_files_todo IN_LIST ABSOLUTE_DEFINITIONS_DEPENDENCIES) if cmake_minimum_required_version >= 3.3
        list(FIND ABSOLUTE_DEFINITIONS_DEPENDENCIES "${current_deffile}" todo_index)
        if(todo_index LESS 0)
            list(APPEND ABSOLUTE_DEFINITIONS_DEPENDENCIES "${current_deffile}")
            file(READ "${current_deffile}" contents_deffile)
            set(include_regex  "(^|\n)include[ \t]+\"([^\"]+)\"")
            string(REGEX MATCHALL "${include_regex}" includes_contents_match "${contents_deffile}")
            foreach(include_match ${includes_contents_match})
                string(REGEX MATCH "${include_regex}" _m "${include_match}")
                set(include_def_file "${CMAKE_MATCH_2}")
                if(IS_ABSOLUTE "${include_def_file}")
                    set(abs_include_def_file "${include_def_file}")
                else()
                    set(abs_include_def_file)
                    foreach(absolute_flatcc_path ${absolute_flatcc_paths})
                        if(NOT abs_include_def_file)
                            if(EXISTS "${absolute_flatcc_path}/${include_def_file}")
                                set(abs_include_def_file "${absolute_flatcc_path}/${include_def_file}")
                            endif()
                        endif()
                    endforeach()
                    if(NOT abs_include_def_file)
                        message(WARNING "${current_deffile} includes ${include_def_file}, but cannot be found in search path")
                    endif()
                endif()
                if(abs_include_def_file)
                    list(APPEND absolute_definition_files_todo "${abs_include_def_file}")
                endif()
            endforeach()
        endif()
        list(REMOVE_AT absolute_definition_files_todo 0)
    endwhile()

    list(REMOVE_DUPLICATES ABSOLUTE_DEFINITIONS_DEPENDENCIES)
    list(REMOVE_DUPLICATES ABSOLUTE_DEFINITION_FILES)

    set(OUTPUT_FILES)
    if(FLATCC_OUTFILE)
        list(APPEND FLATCC_COMPILE_FLAGS "--outfile=${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
        list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
    else()
        if(FLATCC_RECURSIVE)
            set(definition_files ${ABSOLUTE_DEFINITIONS_DEPENDENCIES})
        else()
            set(definition_files ${ABSOLUTE_DEFINITION_FILES})
        endif()
        foreach(definition_file ${definition_files})
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

    message(VERBOSE "---- flatcc start ----")
    message(VERBOSE "output directory: ${FLATCC_OUTPUT_DIR}")
    message(VERBOSE "output files: ${OUTPUT_FILES}")
    message(VERBOSE "execute: flatcc;${FLATCC_COMPILE_FLAGS};${ABSOLUTE_DEFINITION_FILES}")
    message(VERBOSE "dependencies: ${ABSOLUTE_DEFINITIONS_DEPENDENCIES}")
    message(VERBOSE "----- flatcc end -----")

    add_custom_command(OUTPUT ${OUTPUT_FILES}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${FLATCC_OUTPUT_DIR}"
        COMMAND flatcc::cli ${FLATCC_COMPILE_FLAGS} ${ABSOLUTE_DEFINITION_FILES}
        DEPENDS flatcc::cli ${ABSOLUTE_DEFINITIONS_DEPENDENCIES}
    )
    if(FLATCC_TARGET)
        # TODO: possible improvement if cmake_minimum_required_version >= 3.1:
        #        use add_library(... INTERFACE) +  add_custom_command + target_sources(INTERFACE) + target_link_libraries()
        add_custom_target("${FLATCC_TARGET}" DEPENDS ${OUTPUT_FILES})
    endif()

endfunction()
