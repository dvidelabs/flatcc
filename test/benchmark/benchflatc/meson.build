name = 'benchflatc'

# Testing Googles flatc implementation
benchflatc = executable(name, name + '.cpp',
        # Sort of like default_options: ['cpp_std=c++11'] in main
        # project def, but we don't want the main project to be
        # to enable cpp just for this one test.
        cpp_args: ['-std=c++11'], # get NDEBUG flag
        include_directories : testinc_dir)

benchmark(name, benchflatc)
