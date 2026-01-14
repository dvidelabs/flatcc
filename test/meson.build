test_dir = meson.current_source_dir()
bench_dir = join_paths(test_dir, 'benchmark')

testinc_dir = [ inc_dir, include_directories('benchmark/benchmain')]

monster_test_dir = join_paths(test_dir, 'monster_test')

# This schema is widely reused in various tests and has
# include statements unlike the samples/monster/monster.fbs schema.
monster_test_fbs = join_paths(monster_test_dir, 'monster_test.fbs')

# For compatibility tests:
# Google flatc generated binary FlatBuffer from C++ code generator.
monster_data_test_bin_src = join_paths(test_dir, 'flatc_compat/monsterdata_test.mon')

# Google flatc printed json from the binary FlatBuffer.
monster_data_test_json_ref = join_paths(test_dir, 'flatc_compat/monsterdata_test.golden')

flatbench_fbs = join_paths(bench_dir, 'schema/flatbench.fbs')

subdir('monster_test')

# Simply testing for debugging build rules and for smoke testing new platforms.
if get_option('xflatcc_monster_test_only')
  subdir_done()
endif

testdirs = [
  'cgen_test',
  'monster_test_cpp',
  'monster_test_solo',
  'monster_test_prefix',
  'monster_test_concat',
  'emit_test',
  'load_test',
  'json_test',
  'flatc_compat'
]

if get_option('flatcc_reflection')
  testdirs += 'reflection_test'
else
  warning('reflection disabled')
endif

testdirs += [
  'benchmark/benchflatcc',
  'benchmark/benchflatc',
  'benchmark/benchraw',
  'benchmark/benchflatccjson'
]

foreach testdir : testdirs
  subdir(testdir)
endforeach

# As of meson 0.34 the benchmark loops over the test program incl. warmup
# and dumps output in a condensed json file - so we add a run_target to
# drive the test instead.
if meson.backend() == 'ninja'
  message('Note: run benchmark with `ninja flatbench`, not `ninja benchmark`')
endif

run_target('flatbench',
  depends: [benchflatcc, benchflatc, benchraw, benchflatccjson],
  command: [
    benchflatcc, '&&',
    benchflatc, '&&',
    benchraw, '&&',
    benchflatccjson
  ]
)
