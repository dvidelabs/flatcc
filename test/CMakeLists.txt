# Note: some files under source control may be tested with binary comparison.
# Under git such files are protected with the `.gitattributes` file.
# Incorrect line endings may lead to failed tests.
if (FLATCC_TEST)
add_subdirectory(cgen_test)
add_subdirectory(monster_test)
add_subdirectory(monster_test_solo)
add_subdirectory(monster_test_concat)
add_subdirectory(monster_test_prefix)
add_subdirectory(flatc_compat)
add_subdirectory(json_test)
add_subdirectory(emit_test)
add_subdirectory(load_test)
# Reflection can break during development, so it is necessary
# to disable until new reflection code generates cleanly.
if (FLATCC_REFLECTION)
    add_subdirectory(reflection_test)
endif()
endif()
