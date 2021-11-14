#define APPROVALS_CATCH // This tells Approval Tests to provide a main() - only
                        // do this in one cpp file
#include "ApprovalTests.hpp"

[[maybe_unused]] auto directory_disposer =
    ApprovalTests::Approvals::useApprovalsSubdirectory("approval_tests");
