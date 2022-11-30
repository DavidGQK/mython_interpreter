cmake_minimum_required(VERSION 3.5)

project(Mython LANGUAGES CXX)

option (TESTING "Compile and run tests" ON)

set (lexer
    "include/lexer.h"
    "src/lexer.cpp")

set (runtime
    "include/runtime.h"
    "src/runtime.cpp")

set (statement
    "include/statement.h"
    "src/statement.cpp")

set (parse
    "include/parse.h"
    "src/parse.cpp")

set (mython "src/mython.cpp" ${lexer} ${runtime} ${statement} ${parse})

add_executable(Mython ${mython})
target_include_directories(Mython PRIVATE "include")

set_target_properties(Mython PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

if (TESTING)

    set (test_utils
        "include/test_runner_p.h")

    set(lexer_test
        "src/lexer_test.cpp"
        "src/lexer_test_exec.cpp")

    set (runtime_test
        "src/runtime_test.cpp"
        "src/runtime_test_exec.cpp")

    set (statement_test
        "src/statement_test.cpp"
        "src/statement_test_exec.cpp")

    set (parse_test
        "src/parse_test.cpp"
        "src/parse_test_exec.cpp")

    add_executable(Lexer ${lexer} ${lexer_test} ${test_utils})
    target_include_directories(Lexer PRIVATE "include")

    add_executable(Runtime ${runtime} ${runtime_test} ${test_utils})
    target_include_directories(Runtime PRIVATE "include")

    add_executable(Statement ${statement} ${runtime} ${statement_test} ${test_utils})
    target_include_directories(Statement PRIVATE "include")

    add_executable(Parse ${parse} ${lexer} ${runtime} ${statement} ${parse_test} ${test_utils})
    target_include_directories(Parse PRIVATE "include")

    set_target_properties(Lexer Runtime Statement Parse PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )

    if (MSVC)
        add_compile_options(/W3 /WX)
    else ()
        add_compile_options(-Werror -Wall)
    endif ()

    enable_testing()

    add_test (Lexer_Tests Lexer)
    add_test (Runtime_Tests Runtime)
    add_test (Statement_Tests Statement)
    add_test (Parse_Tests Parse)
    set_tests_properties (Lexer_Tests Runtime_Tests Statement_Tests Parse_Tests PROPERTIES
        PASS_REGULAR_EXPRESSION "OK"
        FAIL_REGULAR_EXPRESSION "fail")

endif ()
