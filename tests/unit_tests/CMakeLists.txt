include("../../cmake/CPM.cmake")

CPMAddPackage("gh:catchorg/Catch2@3.7.1")
CPMAddPackage("gh:fmtlib/fmt#11.0.2")

add_executable(mcc_unit_tests
        arenas.hpp
        arenas.cpp
        arena_test.cpp
        string_test.cpp
        formatting_test.cpp
        lexer_test.cpp
        dynarray_test.cpp
        line_numbers_test.cpp
)
target_link_libraries(mcc_unit_tests PUBLIC mcc_lib mcc::compiler_warnings Catch2::Catch2WithMain fmt::fmt)

add_test(NAME mcc COMMAND mcc_unit_tests)
