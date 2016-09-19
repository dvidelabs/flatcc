name = 'benchflatcc'

fb_flatbench = flatcc_json_gen.process(flatbench_fbs)

benchflatcc = executable(name, name + '.c',
        fb_flatbench, flatcc_common, link_with : libflatccrt_sta,
        c_args: test_c_args, # get NDEBUG flag
        include_directories : testinc_dir)

benchmark(name, benchflatcc)
