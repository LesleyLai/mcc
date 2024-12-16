pub struct TestRunnerStates {
    pub failed_test_count: usize,
    pub total_test_count: usize,
}

impl TestRunnerStates {
    pub fn new() -> Self {
        Self {
            failed_test_count: 0,
            total_test_count: 0,
        }
    }
}
