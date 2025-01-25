use crate::global_configuration::global_config;
use crate::test_configuration::TestConfig;
use crate::test_database::{PathHandle, TestDatabase};
use serde::Deserialize;
use std::collections::HashMap;
use std::fs::{read_dir, File};
use std::io::Read;
use std::path::Path;

#[derive(Deserialize, Debug)]
struct TestTomlConfig {
    command: String,

    #[serde(default)]
    return_code: i32,

    #[serde(default)]
    snapshot_test_stderr: bool,
}

#[derive(Deserialize, Debug)]
#[serde(untagged)]
enum TestTomlFile {
    Flat(TestTomlConfig),
    Nested {
        commands: HashMap<String, TestTomlConfig>,
    },
}

fn detect_tests_with_command(
    database: &mut TestDatabase,
    current_dir_handle: PathHandle,
    toml_config: &TestTomlConfig,
    suffix: &str,
) {
    let global_config = global_config();
    let mcc_path = &global_config.mcc_path;

    let command = toml_config
        .command
        .replace("{mcc}", &mcc_path.display().to_string());
    let command = database.add_command(command);

    let suffix = database.add_string(suffix.to_string());

    let current_dir = database.get_path(current_dir_handle);

    for entry in read_dir(current_dir).unwrap() {
        let path_handle = database.add_path(entry.unwrap().path());
        let path = database.get_path(path_handle);

        if path.extension().is_some_and(|e| e == "c") {
            let config = TestConfig {
                command,
                path: path_handle,
                expect_code: toml_config.return_code,
                working_dir: current_dir_handle,
                snapshot_test_stderr: toml_config.snapshot_test_stderr,
                suffix,
            };

            database.add_test(config);
        }
    }
}

fn read_test_config_file(directory: &Path) -> Option<TestTomlFile> {
    use std::io::ErrorKind;

    let file_path = directory.join("test_config.toml");
    let file = File::open(&file_path);
    if file
        .as_ref()
        .is_err_and(|e| e.kind() == ErrorKind::NotFound)
    {
        return None;
    }
    let mut file = file.unwrap();

    let mut s = String::new();
    file.read_to_string(&mut s)
        .expect("Failed to read test toml");

    toml::from_str(&s).expect("Failed to parse test toml")
}

fn detect_tests_in(
    database: &mut TestDatabase,
    current_dir_handle: PathHandle,
    parent_config_file: Option<&TestTomlFile>,
) {
    let current_dir = database.get_path(current_dir_handle);

    let config_file = read_test_config_file(current_dir);
    let config_file = config_file.as_ref().or(parent_config_file);
    for entry in read_dir(current_dir).unwrap() {
        let path = entry.unwrap().path();
        if path.is_dir() {
            let path_handle = database.add_path(path);
            detect_tests_in(database, path_handle, config_file);
        }
    }

    if let Some(config_file) = config_file {
        match config_file {
            TestTomlFile::Flat(config) => {
                detect_tests_with_command(database, current_dir_handle, &config, "")
            }
            TestTomlFile::Nested { commands } => {
                for (name, command) in commands {
                    detect_tests_with_command(database, current_dir_handle, command, name)
                }
            }
        }
    }
}

pub fn detect_tests() -> TestDatabase {
    let mut database = TestDatabase::new();

    let base_folder = database.add_path(global_config().base_dir.clone());

    detect_tests_in(&mut database, base_folder, None);

    database
}
