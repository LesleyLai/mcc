find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(ApprovalTests REQUIRED)

add_executable(mcc_test main.cpp lexer_test.cpp integration_test.cpp arena_test.cpp ast_printer.cpp ast_printer.hpp)
target_link_libraries(mcc_test PUBLIC mcc_lib Catch2::Catch2 fmt::fmt ApprovalTests::ApprovalTests)

