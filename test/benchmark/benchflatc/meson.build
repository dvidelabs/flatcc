name = 'benchflatc'

add_languages('cpp')

benchflatc_inc_dir = include_directories(
    meson.source_root() + '/include',
    bench_dir + '/benchmain')

# Testing Googles flatc implementation
benchflatc = executable(name, name + '.cpp',
        # Sort of like default_options: ['cpp_std=c++11'] in main
        # project def, but we don't want the main project to be
        # to enable cpp just for this one test.
        cpp_args: ['-std=c++11', test_c_args], # get NDEBUG flag
        include_directories : benchflatc_inc_dir)

benchmark(name, benchflatc)
