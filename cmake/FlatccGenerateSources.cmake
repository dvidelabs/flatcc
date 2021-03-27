# Use `flatcc_generate_sources` to generate C source files from flatbuffer schema files.
# The sources will be generated at build time, and only if some target has the output target
# as a dependency or adds the generated sources.
#
#   flatcc_generate_sources(
#      NAME <name>
#      SCHEMA_FILES <schema-file> [<schema-file> [...]]
#      [ALL] [BINARY_SCHEMA] [COMMON] [COMMON_READER] [COMMON_BUILDER] [BUILDER] [READER]
#      [OUTPUT_DIR <output-directory>]
#      [VERIFIER] [JSON_PARSER] [JSON_PRINTER] [JSON] [RECURSIVE]
#      [OUTFILE <output-file>] [PREFIX <prefix>]
#      [PATHS <include-path> [<include-path> [...]]]
#      [EXTRA_ARGS <flatcc-flag> [<flatcc-flag> [...]]]
#   )
#
# The following things are returned:
#
#   <name>_GENERATED_SOURCES
#
#     Variable containing a list of all generated sources.
#     This variable can be added to `add_library`/`add_executable`.
#
#   flatcc_generated::<name>
#
#     Target that contains all headers. Users must link to it.
#     When linking to it, the headers will be generated.
#     The folder containing these headers will also be added to the include path.
#
# Required arguments:
#
#   NAME <name>
#
#     Unique name. It is used for output variable(s) and target(s).
#
#   SCHEMA_FILES <schema-file> [<schema-file> [...]]
#
#     Flatcc will generate sources for all schema files listed here.
#     This function also track included files, so it is not needed to list all dependent files here.
#     Because the search for included files happens at configure time,
#     the schema files must be available before calling this function.
#
# Optional arguments:
#
#   OUTPUT_DIR <output-directory>
#
#     All generated sources will be written in the `output-directory` folder.
#
#   ALL
#   BINARY_SCHEMA
#   COMMON
#   COMMON_READER
#   COMMON_BUILDER
#   BUILDER
#   READER
#   VERIFIER
#   JSON_PARSER
#   JSON_PRINTER
#   JSON
#   RECURSIVE
#
#     When specified, these will instruct flatcc to generate a specific type of source code.
#     It is preferable to use these options instead of adding flatcc arguments to COMPILE_DEFINITIONS.
#     This is because the generated sources will have correct dependency information.
#     When none of these are specified, the default is READER.
#     These options can be combined.
#
#   OUTFILE <output-file>
#
#     Write all source into one file named `output-file`.
#
#   PREFIX <prefix>
#
#     Prefix all symbols with `prefix`
#
#   PATHS <include-path> [<include-path> [...]]
#
#     Add extra include search paths where flatcc should look for included defintion files.
#
#   EXTRA_ARGS <flatcc-arg> [<flatcc-arg> [...]]
#
#     Add extra arguments to flatcc.
#     Use the arguments `ALL`/`COMMON`/`READER`/`JSON` instead of specifying `-a`/`-c`/`--reader`/`--json` here.
#

cmake_minimum_required(VERSION 3.3)

# TODO: not needed when cmake_minimum_required_version >= 3.4
include(CMakeParseArguments)

function(flatcc_generate_sources)
    # parse function arguments
    set(output_options BINARY_SCHEMA COMMON COMMON_READER COMMON_BUILDER BUILDER READER VERIFIER JSON_PARSER JSON_PRINTER JSON)
    set(NO_VAL_ARGS ALL RECURSIVE ${output_options})
    set(SINGLE_VAL_ARGS NAME OUTPUT_DIR OUTFILE PREFIX)
    set(MULTI_VAL_ARGS SCHEMA_FILES EXTRA_ARGS PATHS)

    cmake_parse_arguments(FLATCC "${NO_VAL_ARGS}" "${SINGLE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})
    
    # TODO: add following check when cmake_minimum_required_version >= 3.15
    #if (FLATCC_KEYWORDS_MISSING_VALUES)
    #    message(FATAL_ERROR "The following keywords in call to flatcc_generate_sources have no value: ${FLATCC_KEYWORDS_MISSING_VALUES}")
    #endif()

    if (FLATCC_UNPARSED_ARGUMENTS)
        message(WARNING "The following unknown keywords in call to flatcc_generate_sources are ignored: ${FLATCC_UNPARSED_ARGUMENTS}")
    endif()

    if (NOT FLATCC_NAME)
        message(FATAL_ERROR "NAME not provided in call to flatcc_generate_sources")
    endif()
    
    if (NOT FLATCC_SCHEMA_FILES)
        message(FATAL_ERROR "No flatbuffer schema files provided")
    endif()

    if (NOT FLATCC_OUTPUT_DIR)
        set(FLATCC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    if (NOT IS_ABSOLUTE "${FLATCC_OUTPUT_DIR}")
        set(FLATCC_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${FLATCC_OUTPUT_DIR}")
    endif()

    list(APPEND FLATCC_ARGS -o "${FLATCC_OUTPUT_DIR}")

    # Add current source directory for finding dependencies
    set(absolute_flatcc_paths "${CMAKE_CURRENT_SOURCE_DIR}")

    set(ABSOLUTE_SCHEMA_FILES)
    foreach(schema_file ${FLATCC_SCHEMA_FILES})
        # TODO: file(REAL_PATH) if cmake_minimum_required_version >= 3.19
        if(IS_ABSOLUTE "${schema_file}")
            set(absolute_def_file "${schema_file}")
        else()
            set(absolute_def_file "${CMAKE_CURRENT_SOURCE_DIR}/${schema_file}")
        endif()
        list(APPEND ABSOLUTE_SCHEMA_FILES "${absolute_def_file}")

        get_filename_component(absolute_def_directory "${absolute_def_file}" DIRECTORY)
        list(APPEND absolute_flatcc_paths "${absolute_def_directory}")
    endforeach()

    foreach(path ${FLATCC_PATHS})
        # TODO: file(REAL_PATH) if cmake_minimum_required_version >= 3.19
        if(NOT IS_ABSOLUTE "${path}")
            set(path "${CMAKE_CURRENT_SOURCE_DIR}/${path}")
        endif()
        list(APPEND absolute_flatcc_paths "${path}")
        list(APPEND FLATCC_ARGS -I "${path}")
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
        list(APPEND FLATCC_ARGS "--prefix=${FLATCC_PREFIX}")
    endif()

    # Handle reader option first as other option encompass reader
    if (FLATCC_READER)
        list(APPEND FLATCC_ARGS --reader)
    endif()
    if (FLATCC_COMMON_READER)
        list(APPEND FLATCC_ARGS --common_reader)
    endif()
    if (FLATCC_COMMON_BUILDER)
        list(APPEND FLATCC_ARGS --common_builder)
    endif()
    if (FLATCC_BUILDER)
        list(APPEND FLATCC_ARGS --builder)
        # Builder also generates reader
        set(FLATCC_READER ON)
    endif()
    if (FLATCC_VERIFIER)
        list(APPEND FLATCC_ARGS --verifier)
        # verifier also generates reader
        set(FLATCC_READER ON)
    endif()
    if (FLATCC_JSON_PARSER)
        list(APPEND FLATCC_ARGS --json-parser)
    endif()
    if (FLATCC_JSON_PRINTER)
        list(APPEND FLATCC_ARGS --json-printer)
    endif()
    if (FLATCC_BINARY_SCHEMA)
        list(APPEND FLATCC_ARGS --schema)
    endif()
    if (FLATCC_RECURSIVE)
        list(APPEND FLATCC_ARGS --recursive)
    endif()

    # handle 'all', 'common', 'json' last as they encompass other options
    if (FLATCC_COMMON)
        list(APPEND FLATCC_ARGS --common)
        set(FLATCC_COMMON_READER ON)
        set(FLATCC_COMMON_BUILDER ON)
    endif()
    if (FLATCC_JSON)
        list(APPEND FLATCC_ARGS --json)
        set(FLATCC_JSON_PARSER ON)
        set(FLATCC_JSON_PRINTER ON)
    endif()
    if (FLATCC_ALL)
        list(APPEND FLATCC_ARGS -a)
        set(FLATCC_COMMON_BUILDER ON)
        set(FLATCC_COMMON_READER ON)
        set(FLATCC_BUILDER ON)
        set(FLATCC_VERIFIER ON)
        set(FLATCC_READER ON)
        set(FLATCC_RECURSIVE ON)
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
    if(FLATCC_BINARY_SCHEMA)
        list(APPEND GENERATED_FILE_SUFFIXES .bfbs)
    endif()

    # grep each schema file recursively for includes, convert them to absolute paths and add them to a list
    set(ABSOLUTE_DEFINITIONS_DEPENDENCIES)
    set(absolute_schema_files_todo ${ABSOLUTE_SCHEMA_FILES})
    while(absolute_schema_files_todo)
        list(GET absolute_schema_files_todo 0 current_deffile)
        # TODO: use if(absolute_schema_files_todo IN_LIST ABSOLUTE_DEFINITIONS_DEPENDENCIES) if cmake_minimum_required_version >= 3.3
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
                    list(APPEND absolute_schema_files_todo "${abs_include_def_file}")
                endif()
            endforeach()
        endif()
        list(REMOVE_AT absolute_schema_files_todo 0)
    endwhile()

    list(REMOVE_DUPLICATES ABSOLUTE_DEFINITIONS_DEPENDENCIES)
    list(REMOVE_DUPLICATES ABSOLUTE_SCHEMA_FILES)

    set(OUTPUT_FILES)
    if(FLATCC_OUTFILE)
        list(APPEND FLATCC_ARGS "--outfile=${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
        list(APPEND OUTPUT_FILES "${FLATCC_OUTPUT_DIR}/${FLATCC_OUTFILE}")
    else()
        if(FLATCC_RECURSIVE)
            set(SCHEMA_FILES ${ABSOLUTE_DEFINITIONS_DEPENDENCIES})
        else()
            set(SCHEMA_FILES ${ABSOLUTE_SCHEMA_FILES})
        endif()
        foreach(schema_file ${SCHEMA_FILES})
            # TODO: should be NAME_WLE, but not supported in cmake 2.8
            get_filename_component(def_name_we "${schema_file}" NAME_WE)
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

    list(APPEND FLATCC_ARGS ${FLATCC_EXTRA_ARGS})

    # TODO: VERBOSE was added in cmake 3.15.
    if(NOT (CMAKE_VERSION VERSION_LESS 3.15))
        message(VERBOSE "---- flatcc info start ----")
        message(VERBOSE "output directory: ${FLATCC_OUTPUT_DIR}")
        message(VERBOSE "output files: ${OUTPUT_FILES}")
        message(VERBOSE "execute: flatcc;${FLATCC_ARGS};${ABSOLUTE_SCHEMA_FILES}")
        message(VERBOSE "dependencies: ${ABSOLUTE_DEFINITIONS_DEPENDENCIES}")
        message(VERBOSE "----- flatcc info end -----")
    endif()

    file(MAKE_DIRECTORY "${FLATCC_OUTPUT_DIR}")

    add_custom_command(OUTPUT ${OUTPUT_FILES}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${FLATCC_OUTPUT_DIR}"
        COMMAND flatcc::cli_native ${FLATCC_ARGS} ${ABSOLUTE_SCHEMA_FILES}
        DEPENDS flatcc::cli_native ${ABSOLUTE_DEFINITIONS_DEPENDENCIES}
    )

    add_custom_target("flatcc_generated_${FLATCC_NAME}"
        DEPENDS ${OUTPUT_FILES})

    add_library("__flatcc_generated_${FLATCC_NAME}" INTERFACE)
    target_include_directories("__flatcc_generated_${FLATCC_NAME}" INTERFACE "${FLATCC_OUTPUT_DIR}")
    add_dependencies("__flatcc_generated_${FLATCC_NAME}" "flatcc_generated_${FLATCC_NAME}")

    add_library("flatcc_generated::${FLATCC_NAME}" ALIAS "__flatcc_generated_${FLATCC_NAME}")
    set("${FLATCC_NAME}_GENERATED_SOURCES" ${OUTPUT_FILES} PARENT_SCOPE)
endfunction()
