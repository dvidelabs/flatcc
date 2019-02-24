project('flatcc', 'c', 'cpp', # cpp only used for test cases
    version : '0.6.0-pre',
    license : 'Apache-2.0',
    default_options : ['c_std=c11', 'cpp_std=c++11', 'default_library=static'],
    meson_version : '>=0.41.0')

buildtype = get_option('buildtype')

# The monster example breaks with msvc2010 due to variable declarations not at block start
# msvc 2010 has version 16.* an 2013 has version 18.*
if meson.get_compiler('c').get_id() == 'msvc'
    has_c99_var_decl = meson.get_compiler('c').version().version_compare('>=18')
else
    has_c99_var_decl = get_option('c_std') != 'c89'
endif

# Meson 0.34 does not support NDEBUG
#
# Test cases do not use global arguments by design and therefore miss
# the NDEBUG flag, but we add test_c_args to benchmarks (25+% slowdown
# without).
test_c_args = []
c_args = []
if buildtype == 'release' or buildtype == 'minsize'
    if not meson.is_subproject()
        c_args = ['-DNDEBUG']
        test_c_args = c_args
    endif
endif

if get_option('xflatcc_gen_version')
revisition_tag = vcs_tag(input : 'config/version.in',
    output : 'revision',
    command : ['git', 'rev-parse', 'HEAD'],
    fallback : 'n/a')

version_tag = vcs_tag(input : 'config/version.in',
    output : 'version',
    command : ['git', 'describe', '--abbrev=0', '--tags'],
    fallback : 'n/a')
endif

inc_dir = include_directories('include', 'config', 'external')

install_subdir('include/flatcc', install_dir: 'include')

if get_option('flatcc_portable') or meson.get_compiler('c').get_id() == 'msvc'
    c_args = c_args + ['-DFLATCC_PORTABLE']
    test_c_args = test_c_args + ['-DFLATCC_PORTABLE']
endif

if get_option('flatcc_debug_verify')
    c_args = c_args + ['-DFLATCC_DEBUG_VERIFY=1']
endif

if get_option('flatcc_fast_double')
    c_args = c_args + ['-DGRISU3_PARSE_ALLOW_ERROR', '-DFLATCC_USE_GRISU3=1']
endif

# Reflection must be disabled temporarily when making breaking changes.
if get_option('flatcc_reflection')
    c_args = c_args + ['-DFLATCC_REFLECTION=1']
else
    c_args = c_args + ['-DFLATCC_REFLECTION=0']
endif

flatccrt_src = [
    'src/runtime/builder.c',
    'src/runtime/emitter.c',
    'src/runtime/refmap.c',
    'src/runtime/json_parser.c',
    'src/runtime/json_printer.c',
    'src/runtime/verifier.c']

flatcc_src = [
    'src/compiler/codegen_c.c',
    'src/compiler/codegen_c_builder.c',
    'src/compiler/codegen_c_json_parser.c',
    'src/compiler/codegen_c_json_printer.c',
    'src/compiler/codegen_c_reader.c',
    'src/compiler/codegen_c_sort.c',
    'src/compiler/codegen_c_sorter.c',
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
    flatccrt_src]


libflatccrt = library('flatccrt', flatccrt_src, install : true,
        c_args : c_args, include_directories : inc_dir)

libflatcc = library('flatcc', flatcc_src, install : true,
            c_args : c_args, include_directories : inc_dir)

flatcc = executable('flatcc', 'src/cli/flatcc_cli.c', install : true,
    link_with : libflatcc,
    c_args : c_args,
    include_directories : inc_dir)

subdir('rules')

if not get_option('flatcc_disable_tests')
if not meson.is_subproject()
    # For end user programs, samples, and test.
    subdir('samples')
    subdir('test')
endif
endif
