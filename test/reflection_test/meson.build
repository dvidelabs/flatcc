name = 'reflection_test'

out_dir = meson.current_build_dir()

# Generate binary schema
bfbs_monster_test = custom_target('monster_test.bfbs',
    output  : '@BASENAME@.bfbs',
    input : monster_test_fbs,
    command : [flatcc, '--schema', '-o', out_dir, '@INPUT@'])

test(name,
    executable(name, name + '.c',
        bfbs_monster_test,
        link_with : libflatcc,
        include_directories : testinc_dir),
    args: out_dir + '/monster_test.bfbs')
