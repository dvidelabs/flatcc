# NOTE: This build system is maintained by the community.
project('flatcc',
  'c', 'cpp',
  version : '0.6.1',
  license : 'Apache-2.0',
  default_options : ['c_std=c11', 'cpp_std=c++11', 'default_library=static'],
  meson_version : '>=0.56.0'
)

buildtype = get_option('buildtype')
ignore_const_cond = get_option('ignore_const_cond')
allow_werror = get_option('allow_werror')
gnu_posix_memalign = get_option('gnu_posix_memalign')
has_c99_var_decl = get_option('c_std') != 'c89'

cc = meson.get_compiler('c')
cc_id = cc.get_id()
cc_version = cc.version()

cxx = meson.get_compiler('cpp')

c_args = []

if get_option('flatcc_portable') or cc_id == 'msvc'
  c_args += '-DFLATCC_PORTABLE'
endif

if get_option('flatcc_debug_verify')
  c_args += '-DFLATCC_DEBUG_VERIFY=1'
endif

if get_option('flatcc_fast_double')
  c_args += '-DGRISU3_PARSE_ALLOW_ERROR'
  c_args += '-DFLATCC_USE_GRISU3=1'
endif

if buildtype == 'release' or buildtype == 'minsize'
  c_args += '-DNDEBUG'
endif

if cc_id == 'mvsc'

  has_c99_var_decl = cc_version.version_compare('>=18')
  c_args += '-W3'

elif cc_id == 'clang'

  c_args += [
    '-Wstrict-prototypes',
    '-Wsign-conversion',
    '-Wconversion',
    '-pedantic',
    '-Wextra'
  ]

  if ignore_const_cond
    c_args += '-Wno-tautological-constant-out-of-range-compare'
  endif

  if cc_version.version_compare('< 6')
    c_args += '-Wno-missing-field-initializers'
  endif

elif cc_id == 'gcc'

  c_args += [
    '-Wall',
    '-Wextra',
  ]

  if ignore_const_cond
    c_args += '-Wno-type-limits'
  endif

  if cc_version.version_compare('>4.8')
    c_args += '-Wsign-compare'
  endif

  if cc_version.version_compare('>8')
    c_args += [ '-Wno-stringop-truncation', '-Wno-format-overflow' ]
  endif

  if cc_version.version_compare('>=11')
    c_args += '-Wno-misleading-indentation'
  endif

  if gnu_posix_memalign
    c_args += '-DPORTABLE_POSIX_MEMALIGN=1'
  endif

endif

if build_machine.system() == 'windows'
  c_args += '-D_CRT_SECURE_NO_WARNINGS'
endif

if allow_werror
 c_args += '-Werror'
endif

# Reflection must be disabled temporarily when making breaking changes.
if get_option('flatcc_reflection')
  c_args += '-DFLATCC_REFLECTION=1'
else
  c_args += '-DFLATCC_REFLECTION=0'
endif

add_project_arguments(cc.get_supported_arguments(c_args), language: 'c')
add_project_arguments(cxx.get_supported_arguments(c_args), language: 'cpp')

inc_dir = include_directories('include', 'config', 'external')

if not meson.is_subproject()
  install_subdir('include/flatcc', install_dir: 'include')
endif

flatccrt_src = [
  'src/runtime/builder.c',
  'src/runtime/emitter.c',
  'src/runtime/refmap.c',
  'src/runtime/json_parser.c',
  'src/runtime/json_printer.c',
  'src/runtime/verifier.c'
]

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
  flatccrt_src
]

flatccrt_name = 'flatccrt'
flatcc_name = 'flatcc'

if buildtype == 'debug' and build_machine.system() != 'windows'
  flatccrt_name = 'flatccrt_d'
  flatcc_name = 'flatcc_d'
endif

libflatccrt = library(flatccrt_name,
  sources: flatccrt_src,
  include_directories: inc_dir,
  install: not meson.is_subproject()
)

libflatcc = library(flatcc_name,
  sources: flatcc_src,
  include_directories: inc_dir,
  install: not meson.is_subproject()
)

flatcc = executable(flatcc_name,
  sources: ['src/cli/flatcc_cli.c'],
  link_with: libflatcc,
  include_directories: inc_dir,
  install: not meson.is_subproject()
)

if not meson.is_subproject() and build_machine.system() != 'windows'

  mkdir_command = find_program('mkdir')
  cp_command = find_program('cp')

  copy_targets = {
    'flatcc-copy' : [flatcc, meson.project_source_root() / 'bin'],
    'libflatccrt-copy' : [libflatccrt, meson.project_source_root() / 'lib'],
    'libflatcc-copy' : [libflatcc, meson.project_source_root() / 'lib']
  }

  foreach target_name, target_details : copy_targets
    target = target_details[0]
    destination = target_details[1]
    custom_target(target_name,
      command: [ mkdir_command, '-p', destination, '&&', cp_command, target, destination ],
      output: 'fake-@0@'.format(target_name),
      build_by_default: true,
      build_always_stale: false
    )
  endforeach
endif

meson.override_find_program('flatcc', flatcc)

subdir('rules')

if not get_option('flatcc_disable_tests') and not meson.is_subproject()
  # For end user programs, samples, and test.
  subdir('samples')
  subdir('test')
endif

flatcc_dep = declare_dependency(
  link_with: libflatcc,
  include_directories: inc_dir
)

