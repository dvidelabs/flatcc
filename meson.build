project('flatcc', 'c',
    version : '0.3.5-pre',
    license : 'Apache-2.0',

    # For test, custom_target 'capture' was introduced here.
    meson_version : '>=0.34.0')

buildtype = get_option('buildtype')

# Meson 0.34 does not support NDEBUG
#
# Test cases do not use global arguments by design and therefore miss
# the NDEBUG flag, but we add test_c_args to benchmarks (25+% slowdown
# without).
if buildtype == 'release' or buildtype == 'minsize'
    test_c_args = ['-DNDEBUG']
    add_global_arguments('-DNDEBUG', language : 'c')
else
    test_c_args = []
endif

# This meson project does not attempt to handle old gcc versions and for the
# time being does not set aggressive warnings that requires extra attention
# to compiler versions. This is more about making the project available
# for other meson projects. The CMake build handles various compiler
# versions more carefully.

inc_dir = include_directories('include', 'config', 'external')

if get_option('flatcc_portable')
    add_global_arguments('-DFLATCC_PORTABLE', language : 'c')
endif

if get_option('flatcc_debug_verify')
    add_global_arguments('-DFLATCC_DEBUG_VERIFY=1', language : 'c')
endif

if get_option('flatcc_fast_double')
    add_global_arguments('-DGRISU3_PARSE_ALLOW_ERROR', '-DFLATCC_USE_GRISU3=1', language : 'c')
endif

# Reflection must be disabled temporarily when making breaking changes.
if get_option('flatcc_reflection')
    add_global_arguments('-DFLATCC_REFLECTION=1', language : 'c')
else
    add_global_arguments('-DFLATCC_REFLECTION=0', language : 'c')
endif

flatccrt_src = [
    'src/runtime/builder.c',
    'src/runtime/emitter.c',
    'src/runtime/json_parser.c',
    'src/runtime/json_printer.c',
    'src/runtime/verifier.c' ]

flatcc_src = [
    'src/compiler/codegen_c.c',
    'src/compiler/codegen_c_builder.c',
    'src/compiler/codegen_c_json_parser.c',
    'src/compiler/codegen_c_json_printer.c',
    'src/compiler/codegen_c_reader.c',
    'src/compiler/codegen_c_sort.c',
    'src/compiler/codegen_c_verifier.c',
    'src/compiler/codegen_schema.c',
    'src/compiler/coerce.c',
    'src/compiler/fileio.c',
    'src/compiler/flatcc.c',
    'src/compiler/parser.c',
    'src/compiler/semantics.c',
    'src/compiler/hash_tables/name_table.c',
    'src/compiler/hash_tables/schema_table.c',
    'src/compiler/hash_tables/scope_table.c',
    'src/compiler/hash_tables/symbol_table.c',
    'src/compiler/hash_tables/value_set.c',
    'external/hash/cmetrohash64.c',
    'external/hash/ptr_set.c',
    'external/hash/str_set.c',
    flatccrt_src ]

# We build both library types because shared libraries are good
# with a lot of different buffers around
# but static libraries are essential for optimizing high performance
# builders and json parsers.
# However, to be nice we also support the 'default_library' convention.
if get_option('flatcc_use_default_library')
    libflatccrt = library('flatccrt', flatccrt_src, include_directories : inc_dir)
    libflatcc = library('flatcc', flatcc_src, include_directories : inc_dir)
else
    libflatccrt_sha = shared_library('flatccrt', flatccrt_src, include_directories : inc_dir)
    libflatccrt_sta = static_library('flatccrt', flatccrt_src, include_directories : inc_dir)
    libflatccrt = libflatccrt_sta

    libflatcc_sha = shared_library('flatcc', flatcc_src, include_directories : inc_dir)
    libflatcc_sta = static_library('flatcc', flatcc_src, include_directories : inc_dir)
    libflatcc = libflatcc_sta
endif


flatcc = executable('flatcc', 'src/cli/flatcc_cli.c',
    link_with : libflatcc,
    include_directories : inc_dir)

if not get_option('flatcc_disable_tests')
if not meson.is_subproject()
    # For end user programs, samples, and test.
    subdir('rules')
    subdir('samples')
    subdir('test')
endif
endif
