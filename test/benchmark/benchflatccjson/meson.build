name = 'benchflatccjson'

fb_flatbench = flatcc_json_gen.process(flatbench_fbs)

benchflatccjson = executable(name, name + '.c',
        fb_flatbench, flatcc_common, link_with : libflatccrt,
        include_directories : testinc_dir)

benchmark(name, benchflatccjson)
