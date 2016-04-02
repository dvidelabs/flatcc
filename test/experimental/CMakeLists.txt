CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)
PROJECT(P NONE)
SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(GrammarSource ${PROJECT_BINARY_DIR}/src)
FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src)

ADD_CUSTOM_TARGET(GrammarBuild ALL
    COMMAND ${CMAKE_COMMAND} -E echo "\${X}"
        > ${GrammarSource}/testLexer.h
    COMMAND ${CMAKE_COMMAND} -E echo "\${X}"
        > ${GrammarSource}/testLexer.c
    COMMAND ${CMAKE_COMMAND} -E echo "\${X}"
        > ${GrammarSource}/testParser.h
    COMMAND ${CMAKE_COMMAND} -E echo "\${X}"
        > ${GrammarSource}/testParser.c
    COMMAND ${CMAKE_COMMAND} -E echo "\${X}"
        > ${PROJECT_BINARY_DIR}/test.tokens
)

ADD_CUSTOM_COMMAND(TARGET GrammarBuild POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GrammarSource}/testLexer.h
        ${PROJECT_BINARY_DIR}/parser/testLexer.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GrammarSource}/testLexer.c
        ${PROJECT_BINARY_DIR}/parser/testLexer.c
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GrammarSource}/testParser.h
        ${PROJECT_BINARY_DIR}/parser/testParser.h
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${GrammarSource}/testParser.c
        ${PROJECT_BINARY_DIR}/parser/testParser.c
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_BINARY_DIR}/test.tokens
        ${PROJECT_BINARY_DIR}/parser/test.tokens
)
