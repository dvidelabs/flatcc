name = 'cgen_test'

test(name,
    executable(name, name + '.c',
        link_with : libflatcc,
        c_args: test_c_args,
        include_directories : testinc_dir))
