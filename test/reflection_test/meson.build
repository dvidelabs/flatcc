name = 'reflection_test'

out_dir = meson.current_build_dir()

if get_option('xflatcc_reflection_test_custom')

bfbs_monster_test = custom_target('monster_test.bfbs',
    output  : 'monster_test.bfbs',
    input : monster_test_fbs,
    depfile : 'monster_test.fbs.d',
    command : [ flatcc, '--schema', '-d', '-o', out_dir, '--depfile', '@DEPFILE@', '@INPUT@' ])

else

# Currently (meson 0.34) it is not possible to generate .bfbs. files
# because there must be a registered compiler that handles .bfbs
# extension.
bfbs_monster_test = flatcc_bfbs_gen.process(monster_test_fbs)

endif

test(name,
    executable(name, name + '.c',
        bfbs_monster_test,
        link_with : libflatcc,
        c_args: test_c_args,
        include_directories : testinc_dir),
    args: out_dir + '/monster_test.bfbs')

