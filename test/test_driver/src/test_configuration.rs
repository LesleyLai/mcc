// Configuration of a single test

use crate::test_database::{CommandHandle, PathHandle, StringHandle, TestDatabase};
use regex::Regex;
use std::fs::File;
use std::io::{BufRead, BufReader};
use std::sync::OnceLock;

#[derive(Copy, Clone)]
pub struct TestConfig {
    pub path: PathHandle,
    pub command: CommandHandle,
    // expected return code
    pub expect_code: i32,
    // Is non-empty when a test file has multiple configurations
    pub suffix: StringHandle,
    pub working_dir: PathHandle,

    // snapshot stderr
    pub snapshot_test_stderr: bool,
}

fn return_regex() -> &'static Regex {
    static RE: OnceLock<Regex> = OnceLock::new();
    RE.get_or_init(|| Regex::new(r"// +RETURN: +([0-9]+)$").unwrap())
}

impl TestConfig {
    // Override test config is finding related instructions in source file
    pub fn override_by_file(&self, database: &TestDatabase, test_file_path: PathHandle) -> Self {
        let mut modified = *self;

        let return_re = return_regex();

        let test_file_path = database.get_path(test_file_path);
        let file = File::open(&test_file_path).unwrap();
        let reader = BufReader::new(file);

        let mut overrided_return = None;
        for line in reader.lines() {
            let line = line.unwrap();
            if let Some(captures) = return_re.captures(&line) {
                overrided_return = Some(captures.get(1).unwrap().as_str().parse().unwrap());
            }
        }

        if let Some(overrided_return) = overrided_return {
            modified.expect_code = overrided_return;
        }

        modified
    }
}
