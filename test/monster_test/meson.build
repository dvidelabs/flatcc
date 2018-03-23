name = 'monster_test'

test(name,
    executable(name, name + '.c',
        flatcc_common,
        flatcc_gen.process(monster_test_fbs),
        link_with : libflatccrt,
        c_args: test_c_args,
        include_directories : testinc_dir))
