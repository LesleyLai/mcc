find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(ApprovalTests REQUIRED)

add_executable(mcc_test
        main.cpp
        lexer_test.cpp
        integration_test.cpp
        ast_printer.cpp
        ast_printer.hpp
        source_location_formatter.hpp
        utils_test/allocator_test.cpp
        utils_test/string_test.cpp cstring_view_format.hpp)
target_link_libraries(mcc_test PUBLIC mcc_lib mcc::compiler_warnings Catch2::Catch2 fmt::fmt ApprovalTests::ApprovalTests)

add_test(NAME mcc COMMAND mcc_test)