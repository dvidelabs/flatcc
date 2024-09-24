name = 'monster_test_solo'

# merges all output to stdout and into a capture file
# test that files are produced as given on the command line.
# Note that -cwv does not recurse the monster_test so dependencies are managed
# manually, unlike -cwvr aka -a on just the main monster_test file
# that latter is tested in the monster_test_prefix test.
prefix_gen = custom_target(name,
  output : 'monster_test.h',
  input : [
    join_paths(monster_test_dir, 'attributes.fbs'),
    join_paths(monster_test_dir, 'include_test2.fbs'),
    join_paths(monster_test_dir, 'include_test1.fbs'),
    join_paths(monster_test_dir, 'monster_test.fbs'),
  ],
  capture : true,
  command : [flatcc, '-cwv', '--stdout', '@INPUT@']
)

test(name,
  executable(name, name + '.c',
    prefix_gen,
    link_with: libflatccrt,
    include_directories: testinc_dir
  )
)
