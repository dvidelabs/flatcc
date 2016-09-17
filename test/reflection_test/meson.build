name = 'reflection_test'

out_dir = meson.current_build_dir()

# Generate binary schema via custom rule
if get_option('xflatcc_reflection_test_custom')

# TODO:
# This version rebuilds repeatedly, the generator does not.
# It might have something to do with full paths in depfile
# target, but it is unclear.
bfbs_monster_test = custom_target('monster_test.bfbs',
    output  : '@BASENAME@' + '.bfbs',
    input : monster_test_fbs,
    depfile: '@BASENAME@' + '.fbs.d',
    command : [flatcc, '--schema', '-do', out_dir,'@INPUT@'])

# ... or via a generator
else
bfbs_monster_test = flatcc_gen.process(monster_test_fbs)
endif

test(name,
    executable(name, name + '.c',
        bfbs_monster_test,
        link_with : libflatcc,
        include_directories : testinc_dir),
    args: out_dir + '/monster_test.bfbs')
