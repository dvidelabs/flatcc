name = 'monster_test_concat'

build_root = meson.build_root()

# merges all output to stdout and into a capture file
# test that files are produced as given on the command line.
# Note that -cwv does not recurse the monster_test so dependencies are managed
# manually, unlike -cwvr aka -a on just the main monster_test file
# that latter is tested in the monster_test_prefix test.

# Here we avoid capture by directing the concatenated output
# to a known file and use it as a dependency target
# without automatic dependency file generation.
#
# NOTE: the build sysem cannot (by design) handle generated output
# files, which cause clean to mess up, but when we send the output to a
# single file, we avoid this problem. The entire set of test cases still
# doesn't clean properly because we need to test various scenarios.
concat_gen = custom_target(name,
    output : 'monster_test.h',
    input : [ monster_test_dir + '/monster_test.fbs' ],
    depfile: '@OUTPUT@.depends',
    command : [flatcc, '-cwvr', '-o', build_root,
        '--depfile', '@DEPFILE@',
        '--outfile', '@OUTPUT@', '@INPUT@'])

# Test manual build of dependent files without recursion.
concat_gen_explicit = custom_target(name + '_explicit',
    output : 'monster_test_explicit.h',
    input : [
        monster_test_dir + '/monster_test.fbs',
        monster_test_dir + '/include_test1.fbs',
        monster_test_dir + '/include_test2.fbs' ],
    command : [
        flatcc, '-cwv', '-o', build_root,
        monster_test_dir + '/include_test2.fbs',
        monster_test_dir + '/include_test1.fbs', '@INPUT@' ])

test(name,
    executable(name, name + '.c',
        prefix_gen,
        link_with : libflatccrt,
        include_directories : testinc_dir))
