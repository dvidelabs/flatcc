name = 'monster_test_cpp'

test(name,
    executable(name, 'monster_test.cpp',
        flatcc_common,
        flatcc_gen.process(monster_fbs),
        link_with : libflatccrt,
        cpp_args: test_c_args,
        include_directories : testinc_dir))
