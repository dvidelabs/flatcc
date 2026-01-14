name = 'monster_test_cpp'

test(name,
  executable(name, 'monster_test.cpp',
    flatcc_common,
    flatcc_gen.process(monster_fbs),
    link_with: libflatccrt,
    include_directories: testinc_dir
  )
)
