name = 'benchflatcc'

fb_flatbench = flatcc_json_gen.process(flatbench_fbs)

benchflatcc = executable(name, name + '.c',
        fb_flatbench, flatcc_common, link_with : libflatccrt,
        c_args: test_c_args,
        include_directories : testinc_dir)

benchmark(name, benchflatcc)
