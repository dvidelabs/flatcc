# Note: some files under source control may be tested with binary comparison.
# Under git such files are protected with the `.gitattributes` file.
# Incorrect line endings may lead to failed tests.
if (FLATCC_TEST)
if (FLATCC_CXX_TEST)
    # This is tests is primarly for making sure C++ users can use
    # generated FlatCC code. It fails for pre GCC 4.7 C++ because both
    # stdint.h and stdalign.h are not sufficiently supported and it
    # is not worth attempting to support in flatcc/portable.
    add_subdirectory(monster_test_cpp)
endif()
add_subdirectory(cgen_test)
add_subdirectory(monster_test)
add_subdirectory(monster_test_solo)
add_subdirectory(monster_test_concat)
add_subdirectory(monster_test_prefix)
add_subdirectory(flatc_compat)
add_subdirectory(json_test)
add_subdirectory(emit_test)
add_subdirectory(load_test)
add_subdirectory(optional_scalars_test)
add_subdirectory(doublevec_test)
# Reflection can break during development, so it is necessary
# to disable until new reflection code generates cleanly.
if (FLATCC_REFLECTION)
    add_subdirectory(reflection_test)
endif()
endif()
