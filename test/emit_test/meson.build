name = 'emit_test'

test(name,
    executable(name, name + '.c',
        flatcc_gen.process(name + '.fbs'), flatcc_common,
        link_with : libflatccrt,
        c_args: test_c_args,
        include_directories : testinc_dir))
