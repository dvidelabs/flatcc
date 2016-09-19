name = 'monster_build_rules'

# We use the predefined flatcc custom target `flatcc_common` to build common
# files once, then use the `flatcc_gen` generator the builds header
# files with reasonable flags without common headers.
# `samples_inc_dir` is includes the directory for the generated common
# files and for the inlcude path where 'flatcc' libary headers live.
#
# These rules makes it fairly simple to ensure build dependencies so
# things are adequately updaeted if, say, a schema included in another
# schema is updated without rebulding thing unnecessarily.

test(name,
    executable(name, monster_src,
        flatcc_common,
        flatcc_gen.process(monster_fbs),
        # See also main meson.build  where both a static and a shared
        # library is available. The static version will likely optimize
        # much better.
        link_with : libflatccrt,
        include_directories : flatcc_include))
