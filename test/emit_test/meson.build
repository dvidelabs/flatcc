name = 'emit_test'

test(name,
    executable(name, name + '.c',
        flatcc_gen.process(name + '.fbs'), flatcc_common,
        link_with : libflatccrt,
        include_directories : testinc_dir))
