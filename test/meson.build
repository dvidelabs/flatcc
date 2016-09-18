build_root = meson.build_root()
test_dir = meson.current_source_dir()
bench_dir = test_dir + '/benchmark'

testinc_dir = [ inc_dir, include_directories(fb_common_dir, bench_dir + '/benchmain')]


# This schema is widely reused in various tests and has
# include statements unlike the samples/monster/monster.fbs schema.
monster_test_dir = test_dir + '/monster_test'
monster_test_fbs = monster_test_dir + '/monster_test.fbs'

# For compatibility tests:
# Google flatc generated binary FlatBuffer from C++ code generator.
monster_data_test_bin_src = test_dir + '/flatc_compat/monsterdata_test.mon'

# Google flatc printed json from the binary FlatBuffer.
monster_data_test_json_ref = test_dir + '/flatc_compat/monsterdata_test.golden'

flatbench_fbs = bench_dir + '/schema/flatbench.fbs'

# Simply testing for debugging build rules and for smoke testing new platforms.
if get_option('xflatcc_monster_test_only')
subdir('monster_test')
else
subdir('cgen_test')
subdir('monster_test')
subdir('monster_test_solo')
subdir('monster_test_prefix')
subdir('monster_test_concat')
subdir('emit_test')
subdir('load_test')
subdir('json_test')
subdir('flatc_compat')

# We disable this during development of breaking changes in code generation.
if get_option('flatcc_reflection')
    subdir('reflection_test')
else
    message('warning: reflection disabled')
endif

subdir('benchmark/benchflatcc')
subdir('benchmark/benchflatc')
subdir('benchmark/benchraw')
subdir('benchmark/benchflatccjson')

# As of meson 0.34 the benchmark loops over the test program incl. warmup
# and dumps output in a condensed json file - so we add a run_target to
# drive the test instead.
message('Run benchmark with `ninja flatbench`, not `ninja benchmark`')

run_target('flatbench',
    depends: [benchflatcc, benchflatc, benchraw, benchflatccjson],
    command: [
        benchflatcc, '&&',
        benchflatc, '&&',
        benchraw, '&&',
        benchflatccjson])
endif # monster_test only
