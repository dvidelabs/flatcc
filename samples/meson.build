monster_dir = meson.current_source_dir() + '/monster'
monster_fbs = monster_dir + '/monster.fbs'
monster_src = monster_dir + '/monster.c'
samples_inc_dir = include_directories('../include', fb_common_dir)

subdir('monster')
subdir('monster_build_rules')
if get_option('flatcc_reflection')
    subdir('reflection')
endif
