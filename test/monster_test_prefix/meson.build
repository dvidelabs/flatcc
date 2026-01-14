name = 'monster_test_prefix'

prefix_name = 'zzz_monster_test'

# merges all output to stdout with a custom namespace prefix,
# meson captures this in the output file (meson 0.34+).

prefix_gen = custom_target(prefix_name,
    output : prefix_name + '.h',
    input : monster_test_fbs,
    capture : true,
    command : [flatcc, '-a', '--prefix=zzz_', '--stdout', '@INPUT@'])

test(name,
    executable(name, name + '.c',
        prefix_gen,
        link_with : libflatccrt,
        include_directories : testinc_dir))
