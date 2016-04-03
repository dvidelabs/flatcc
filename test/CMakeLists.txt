# Note: some files under source control may be tested with binary comparison.
# Under git such files are protected with the `.gitattributes` file.
# Incorrect line endings may lead to failed tests.

add_subdirectory(cgen_test)
add_subdirectory(monster_test)
add_subdirectory(monster_test_solo)
add_subdirectory(monster_test_prefix)
add_subdirectory(flatc_compat)
add_subdirectory(json_test)
if (FLATCC_REFLECTION)
    add_subdirectory(reflection_test)
endif()
add_subdirectory(load_test)
if (NOT MSVC)
    # currently broken on Win32
    add_subdirectory(emit_test)
endif()
