name = 'flatc_compat'

test(name,
    executable(name, name + '.c',
        flatcc_gen.process(monster_test_fbs),
        flatcc_common, link_with : libflatccrt,
        c_args: test_c_args,
        include_directories : testinc_dir),
    args: [monster_data_test_bin_src])
