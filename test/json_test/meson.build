out_dir = meson.current_build_dir()

fb_monster_json = flatcc_json_gen.process(monster_test_fbs)

test('test_basic_parse',
    executable('test_basic_parse', 'test_basic_parse.c',
        fb_monster_json, flatcc_common, link_with : libflatccrt,
        include_directories : testinc_dir))

# Parse a Google flatc printed json file.
test('test_json_parser',
    executable('test_json_parser', 'test_json_parser.c',
        fb_monster_json, flatcc_common, link_with : libflatccrt,
        include_directories : testinc_dir),
    args: [ monster_data_test_json_ref ])

# Print a json file from a Google flatc generated buffer
# and compare the output to a Google flatc printed json file.
test('test_json_printer',
    executable('test_json_printer', 'test_json_printer.c',
        fb_monster_json, flatcc_common, link_with : libflatccrt,
        include_directories : testinc_dir),
    args: [
        monster_data_test_bin_src,
        monster_data_test_json_ref,
        join_paths(out_dir, 'monsterdata_test.json.txt')])

test('test_json',
    executable('test_json', 'test_json.c',
        fb_monster_json, flatcc_common, link_with : libflatccrt,
        include_directories : testinc_dir))

