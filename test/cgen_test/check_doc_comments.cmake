execute_process(
    COMMAND "${CGEN_TEST}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "cgen_test failed (${result}): ${error}")
endif()

set(expected_struct_doc [=[struct the_comment_source {
    alignas(4) int32_t value;
    /**  trailing_doc_comment_should_be_kept
     *  second_trailing_doc_comment_should_be_kept */
};]=])
string(FIND "${output}" "${expected_struct_doc}" trailing_doc_pos)
if(trailing_doc_pos EQUAL -1)
    message(FATAL_ERROR "trailing struct doc comments were not kept inside comment_source")
endif()

set(expected_target_doc [=[/**  following_type_doc_should_remain */
struct the_comment_target {]=])
string(FIND "${output}" "${expected_target_doc}" following_doc_pos)
if(following_doc_pos EQUAL -1)
    message(FATAL_ERROR "doc comment after closing brace was not attached to comment_target")
endif()
