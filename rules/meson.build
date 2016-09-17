fb_common_dir = meson.current_build_dir()


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
    command : [ flatcc, '-cw', '-o', fb_common_dir])

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

# Debugging depfile rules:
#
# Ninja immediately consumes (reads and deletes) depfiles.
# For debugging, rename the file so it does not match, e.g:
# '--depfile=@DEPFILE@,x'
# then `ag -g ,x` in the build dir to find generated depfiles,
# or `find . -type f -name '*,x'` if find is preferable.
#
# With the normal build rules (no rename hack), enable only
# monster_test, the build, then cd build
# ninja
# ninja
# Second time, nothing to build.
# touch ../test/monster_test/include_test2.fbs
# ninja
# should rebuild, next time it shouldn't, try also with touch .../monster.fbs

flatcc_gen = generator(flatcc,
    output  : flatcc_gen_output,
    depfile: '@BASENAME@' + '.fbs.d',
    # with explicit --depfile relative to cwd, i.e. build root,
    # depfile: name can be chosen relatively freely.
    arguments : [ '-vwr', '-o', '@BUILD_DIR@', '--depfile=@DEPFILE@', '@INPUT@' ])
    # With flatcc -d option, depfile: must be exactly as listed
    #arguments : ['-vwrd', '-o', '@BUILD_DIR@', '@INPUT@'])

flatcc_json_gen = generator(flatcc,
    output  : flatcc_json_gen_output,
    depfile: '@BASENAME@' + '.fbs.d',
    arguments : [ '-vwrd', '--json','-o', '@BUILD_DIR@', '@INPUT@' ])

flatcc_bfbs_gen = generator(flatcc,
    output  : '@BASENAME@' + '.bfbs',
    depfile: '@BASENAME@' + '.fbs.d',
    arguments : [ '-rd', '--schema', '-o', '@BUILD_DIR@', '@INPUT@' ])

