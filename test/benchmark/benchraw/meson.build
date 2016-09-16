name = 'benchraw'

benchraw = executable(name, name + '.c',
        c_args: test_c_args, # get NDEBUG flag
        include_directories : testinc_dir)

benchmark(name, benchraw)
