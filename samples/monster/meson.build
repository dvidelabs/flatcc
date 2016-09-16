schema_dir = meson.current_source_dir() 
gen_dir = meson.current_build_dir()
fb_common = ['flatbuffers_common_reader.h', 'flatbuffers_common_builder.h']

monster_gen = custom_target('monster_builder',
    output : ['monster_builder.h', 'monster_reader.h'] + fb_common,
    input : schema_dir + '/monster.fbs',
    command : [ flatcc, '-cw', '-o', gen_dir, '@INPUT@'])

monster_sample = executable('monster', 'monster.c', monster_gen,
    link_with : libflatccrt,
    include_directories : inc_dir)

test('monster_sample', monster_sample)
