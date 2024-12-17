use crate::test_configuration::TestConfig;
use std::path::{Path, PathBuf};

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct PathHandle(u32);

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct CommandHandle(u32);

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub struct StringHandle(u32);

pub struct TestDatabase {
    paths: Vec<PathBuf>,
    tests: Vec<TestConfig>,
    strings: Vec<String>,
}

impl TestDatabase {
    pub fn new() -> Self {
        Self {
            paths: vec![],
            tests: vec![],
            strings: vec![],
        }
    }

    #[must_use]
    pub fn add_path(&mut self, path: PathBuf) -> PathHandle {
        let index = self.paths.len();
        self.paths.push(path);
        PathHandle(u32::try_from(index).expect("path index too large!"))
    }

    #[must_use]
    pub fn get_path(&self, handle: PathHandle) -> &Path {
        &self.paths[handle.0 as usize]
    }

    #[must_use]
    pub fn add_command(&mut self, command: String) -> CommandHandle {
        let index = self.strings.len();
        self.strings.push(command);
        CommandHandle(u32::try_from(index).expect("path index too large!"))
    }

    #[must_use]
    pub fn get_command(&self, handle: CommandHandle) -> &str {
        &self.strings[handle.0 as usize]
    }

    #[must_use]
    pub fn add_string(&mut self, command: String) -> StringHandle {
        let index = self.strings.len();
        self.strings.push(command);
        StringHandle(u32::try_from(index).expect("path index too large!"))
    }

    #[must_use]
    pub fn get_string(&self, handle: StringHandle) -> &str {
        &self.strings[handle.0 as usize]
    }

    pub fn add_test(&mut self, test: TestConfig) {
        self.tests.push(test);
    }

    #[must_use]
    pub fn tests(&self) -> &[TestConfig] {
        &self.tests
    }
}
