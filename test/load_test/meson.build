name = 'load_test'

test(name,
    executable(name, name + '.c',
        flatcc_common,
        flatcc_gen.process(monster_test_fbs),
        link_with : libflatccrt,
        include_directories : testinc_dir))
