# Here we build everything manually.
# Because the schema does not include other
# schema we do not have to deal with additiaonl
# dependencies here.
#
# See also `monster_build_rules` where predefined
# build rules are being used.

if has_c99_var_decl

  my_schema_dir = meson.current_source_dir()
  my_gen_dir = meson.current_build_dir()

  monster_builder_outfiles = [
    'monster_builder.h',
    'monster_reader.h',
    'flatbuffers_common_reader.h',
    'flatbuffers_common_builder.h'  
  ]

  my_monster_gen = custom_target('my_monster_builder',
    output: monster_builder_outfiles,
    input: my_schema_dir + '/monster.fbs',
    command: [ 
      flatcc, '-cw', '-o', my_gen_dir, '@INPUT@'
    ]
  )

  monster_sample = executable('monster', 
    sources: ['monster.c', my_monster_gen ],
    link_with: libflatccrt,
    include_directories: flatcc_include
  )

 test('monster_sample', monster_sample)

else
  message('Skipping monster sample because C compiler does not support late variable declarations')
endif
