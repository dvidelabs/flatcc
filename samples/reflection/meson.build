name = 'bfbs2json'

out_dir = meson.current_build_dir()

# Generate binary schema
bfbs_monster = custom_target('monster.bfbs',
  output: 'monster.bfbs',
  input: monster_fbs,
  command: [flatcc, '--schema', '-o', out_dir, '@INPUT@']
)

test(name,
  executable(name, 
    name + '.c',  bfbs_monster,
    link_with: libflatcc,
    include_directories: flatcc_include
  ),
  args: bfbs_monster.full_path()
)
