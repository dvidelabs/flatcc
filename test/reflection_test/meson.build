name = 'reflection_test'

out_dir = meson.current_build_dir()

if get_option('xflatcc_reflection_test_custom')

bfbs_monster_test = custom_target('monster_test.bfbs',
    output  : 'monster_test.bfbs',
    input : monster_test_fbs,
    depfile : 'monster_test.fbs.d',
    command : [ flatcc, '--schema', '-d', '-o', out_dir, '--depfile', '@DEPFILE@', '@INPUT@' ])

test(name,
    executable(name, name + '.c',
        bfbs_monster_test,
        link_with : libflatcc,
        include_directories : testinc_dir),
    args: out_dir + '/monster_test.bfbs')

else

bfbs_monster_test = flatcc_gen.process(monster_test_fbs)
test(name,
    executable(name, name + '.c',
        bfbs_monster_test,
        link_with : libflatcc,
        include_directories : testinc_dir),
    args: out_dir + '/monster_test.bfbs')

endif
