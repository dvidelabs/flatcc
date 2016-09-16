
# NOTE:
# test cases do not generally use the test_c_args setting we have added
# to work around missing NDEBUG in meson 0.34 (and because tests by
# design don't use added global args), but we do add it in benchmarks.



test_dir = meson.current_source_dir()
gen_dir = meson.current_build_dir()
bench_dir = test_dir + '/benchmark'

testinc_dir = [ inc_dir,
    include_directories(gen_dir, bench_dir + '/benchmain')]


# This schema is widely reused in various tests and has
# include statements unlike the samples/monster/monster.fbs schema.
monster_test_dir = test_dir + '/monster_test'
monster_test_fbs = monster_test_dir + '/monster_test.fbs'

# For compatibility tests:
# Google flatc generated binary FlatBuffer from C++ code generator.
monster_data_test_bin_src = test_dir + '/flatc_compat/monsterdata_test.mon'

# Google flatc printed json from the binary FlatBuffer.
monster_data_test_json_ref = test_dir + '/flatc_compat/monsterdata_test.golden'

flatbench_fbs = bench_dir + '/schema/flatbench.fbs'

# The flatcc compiler can generate common header files and
# schema specific files in one operation, but meson isn't
# allowing us to list them because @BASENAME@ must be in the path.
# It could still work though, but messy. It is better to generate
# the files once. Unforunately we must explicitly add these
# as a dependency to very executable becuase the schema specific
# generator (in meson 0.34) doesn't have a depends: argument.
#
# -c generates header and -w generates builder since we
# don't supply any input .fbs argument -w here means build
# flatbuffers_common_builder.h otherwise we only get the
# common reader.
#
# Common files are generated because it allows for future
# variations in the namespaces and file format, such as
# large offset buffers.
#
fb_common = custom_target('flatbuffer_common',
    output : [
        'flatbuffers_common_reader.h',
        'flatbuffers_common_builder.h'],
    command : [ flatcc, '-cw', '-o', gen_dir])

# flatcc -r generates included schema recursively, and -d lists included
# schema files in a dependency file.  Ninja 1.7.1 (via meson 0.34)
# currently has a bug so either multiple outputs or deps file can be
# chosen, but not both. The best option is to use single output because
# even if the file is not used, the other outputs will be in sync.
# Alternatively the inlcuded files can be listed as inputs explicitly, but
# this is error prone. Regardless, recursive output files cannot be listed
# because Ninja cannot deal with them - this is only important for clean,
# meaning the build dir mus be removed manually or via a helper script.
#
# https://groups.google.com/forum/#!searchin/ninja-build/depslog%7Csort:relevance/ninja-build/4RK3vACeHG0/T3iEPSoLEAAJ
# https://github.com/ninja-build/ninja/issues/1184
#
# However, with single output, single deps file, the build fails this
# step with no detailed explanation (meson 0.34), so we drop the depfile
# for now. This means dependencies must be added manually which we can't
# do in a generator. For our test purposes this is ok, but as an example
# for other users with active schema development, it is not ok.
#
# NOTE: make sure to add fb_common as input to C executables as a
# dependency when using the flatcc_gen generator. This is needed because
# meson does not allow the generator to depend on a custom target as is.
# (We could also just generate the custom headers here with flatcc -c
# option, but they are shared across schema and it is messy to
# update same file repeatedly).

if get_option('xflatcc_multi_dep_targets')
    flatcc_gen_output = [
        '@BASENAME@_reader.h',
        '@BASENAME@_builder.h',
        '@BASENAME@_verifier.h']

    flatcc_json_gen_output = flatcc_gen_output + [
        '@BASENAME@_json_parser.h',
        '@BASENAME@_json_printer.h']
else
    flatcc_gen_output = [ '@BASENAME@_reader.h' ]
    flatcc_json_gen_output = [ '@BASENAME@_reader.h' ]
endif


# NOTE: disable the solo test below when testing depfile because the
# solo test explicitly states dependencies that hides effects of
# scanning. Comment out depfile to debug the result because ninja
# deletes them when it finds them.
flatcc_gen = generator(flatcc,
    output  : flatcc_gen_output,
    #depfile: '@BASENAME@' + '.fbs.d',
    arguments : ['-vwr', '-o', '@BUILD_DIR@',
        #'--depfile', '@DEPFILE@',
        #'--depfile', '@BASENAME@'.format() + '.fbs.d',
        '@INPUT@'])

flatcc_json_gen = generator(flatcc,
    output  : flatcc_json_gen_output,
    #depfile: '@BASENAME@' + '.fbs.d',
    arguments : ['-vwr', '--json','-o', '@BUILD_DIR@', 
            #'--depfile=@DEPFILE@', '--deptarget=@BASENAME@_reader.h',
            #'--depfile=@BASENAME@.fbs.d', '--deptarget=@BASENAME@_reader.h',
            '@INPUT@'])

subdir('cgen_test')
subdir('monster_test')
subdir('monster_test_solo')
subdir('monster_test_prefix')
subdir('monster_test_concat')
subdir('emit_test')
subdir('load_test')
subdir('json_test')
subdir('flatc_compat')


if get_option('flatcc_reflection')
    subdir('reflection_test')
else
    message('warning: reflection disabled')
endif

# Note: benchmark code isn't an ideal match with mesons currrent
# benchmark wrapper as it repeats calling test and includes warmup time.
# but benchmark can also be built as a direct target with ninja.
subdir('benchmark/benchflatcc')
subdir('benchmark/benchflatc')
subdir('benchmark/benchraw')
subdir('benchmark/benchflatccjson')

# As of meson 0.34 the benchmark loops over the test program incl. warmup
# and dumps output in a condensed json file - so we add a run_target to
# drive the test instead.
message('Run benchmark with `ninja flatbench`, not `ninja benchmark`')

run_target('flatbench',
    depends: [benchflatcc, benchflatc, benchraw, benchflatccjson],
    command: [
        benchflatcc, '&&',
        benchflatc, '&&',
        benchraw, '&&',
        benchflatccjson])

