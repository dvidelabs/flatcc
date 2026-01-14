

monster_dir = join_paths(meson.current_source_dir(), 'monster')
monster_fbs = join_paths(monster_dir, 'monster.fbs')
monster_src = join_paths(monster_dir, 'monster.c')

subdir('monster')

if get_option('flatcc_reflection')
  subdir('reflection')
endif
