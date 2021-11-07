#define APPROVALS_CATCH // This tells Approval Tests to provide a main() - only
                        // do this in one cpp file
#include "ApprovalTests.hpp"

auto directoryDisposer =
    ApprovalTests::Approvals::useApprovalsSubdirectory("approval_tests");
