include("../cmake/CPM.cmake")

CPMAddPackage("gh:catchorg/Catch2@3.7.1")
CPMAddPackage("gh:fmtlib/fmt#11.0.2")

add_executable(mcc_test
        utils_test/arena_test.cpp
        utils_test/string_test.cpp)
target_link_libraries(mcc_test PUBLIC mcc_lib mcc::compiler_warnings Catch2::Catch2WithMain fmt::fmt)

add_test(NAME mcc COMMAND mcc_test)