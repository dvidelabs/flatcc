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

if get_option('xflatcc_gen_version')
revisition_tag = vcs_tag(input : 'config/version.in',
    output : 'revision',
    command : [ 'git', 'rev-parse', 'HEAD' ],
    fallback : 'n/a')

version_tag = vcs_tag(input : 'config/version.in',
    output : 'version',
    command : [ 'git', 'describe', '--abbrev=0', '--tags' ],
    fallback : 'n/a')
endif

# This meson project does not attempt to handle old gcc versions and for the
# time being does not set aggressive warnings that requires extra attention
# to compiler versions. This is more about making the project available
# for other meson projects. The CMake build handles various compiler
# versions more carefully.

inc_dir = include_directories('include', 'config', 'external')

# TODO: this likely installs in /usr/local/include/flatcc/flatcc
# but better safe than sorry
install_subdir('include/flatcc', install_dir: 'include')


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

# A static runtime library is really important for flatcc to work well
# so we must use separate names for shared or static libraries, otherwise
# linker options gets messy and won't work at all on OS-X.
libflatccrt = static_library('flatccrt', flatccrt_src, install : true,
        include_directories : inc_dir)

libflatccrt_sha = shared_library('flatccrt_sha', flatccrt_src, install : true,
        include_directories : inc_dir)

# Use defaul_library linkage for the compiler library: We don't care as
# much for the compiler and some package managers might complain.
libflatcc = library('flatcc', flatcc_src, install : true,
        include_directories : inc_dir)

flatcc = executable('flatcc', 'src/cli/flatcc_cli.c', install : true,
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
