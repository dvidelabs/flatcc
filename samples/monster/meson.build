# Here we build everything manually.
# Because the schema does not include other
# schema we do not have to deal with additiaonl
# dependencies here.
#
# See also `monster_build_rules` where predefined
# build rules are being used.

my_schema_dir = meson.current_source_dir() 
my_gen_dir = meson.current_build_dir()


my_monster_gen = custom_target('my_monster_builder',
    output : [
        'monster_builder.h', 'monster_reader.h',
        'flatbuffers_common_reader.h', 'flatbuffers_common_builder.h' ],
    input : my_schema_dir + '/monster.fbs',
    command : [ flatcc, '-cw', '-o', my_gen_dir, '@INPUT@'])

monster_sample = executable('monster', 'monster.c', my_monster_gen,
    link_with : libflatccrt,
    include_directories : samples_inc_dir)

test('monster_sample', monster_sample)
