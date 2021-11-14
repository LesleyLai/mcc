find_package(Catch2 REQUIRED)
find_package(fmt REQUIRED)
find_package(ApprovalTests REQUIRED)

add_executable(mcc_test main.cpp approval_test.cpp arena_test.cpp)
target_link_libraries(mcc_test PUBLIC mcc_lib Catch2::Catch2 fmt::fmt ApprovalTests::ApprovalTests)

