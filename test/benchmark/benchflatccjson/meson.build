name = 'benchflatccjson'

fb_flatbench = flatcc_json_gen.process(flatbench_fbs)

benchflatccjson = executable(name, name + '.c',
        fb_flatbench, fb_common, link_with : libflatccrt,
        c_args: test_c_args, # get NDEBUG flag
        include_directories : testinc_dir)

benchmark(name, benchflatccjson)
