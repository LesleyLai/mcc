mod global_configuration;
mod state;

use colored::Colorize;
use regex::Regex;
use serde::Deserialize;
use std::collections::HashMap;
use std::fmt::Display;
use std::fs::{read_dir, File};
use std::io::{BufRead, BufReader, Read};
use std::path::Path;
use std::process::{exit, Command, ExitCode};
use std::sync::OnceLock;

use crate::global_configuration::global_config;
use crate::state::TestRunnerStates;

fn test_run_mcc() {
    let mcc_path = &global_config().mcc_path;
    let output = Command::new(&mcc_path).output();

    match output {
        Ok(output) => {
            assert!(!output.status.success());
            assert!(String::from_utf8_lossy(&output.stderr)
                .contains("mcc: fatal error: no input files\nUsage: mcc [options] filename..."));
        }
        Err(error) => {
            eprintln!("Failed to run mcc at {}", mcc_path.display());
            eprintln!("Cause: {}", error);
            exit(1);
        }
    }
}

struct TestError {
    expect_code: i32,
    actual_code: Option<i32>, // If no actual code, that means the code is returned as a signal
    stderr: String,
}

impl Display for TestError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        if self
            .actual_code
            .is_none_or(|actual_code| actual_code != self.expect_code)
        {
            write!(
                f,
                "Expected return code: {} Actual return code: ",
                self.expect_code,
            )?;
            match self.actual_code {
                Some(code) => write!(f, "{}", code)?,
                None => write!(f, "none")?,
            }
        }

        if !self.stderr.is_empty() {
            write!(f, "\nWith error message:\n{}", self.stderr)?;
        }
        Ok(())
    }
}

struct TestConfig<'a> {
    command: &'a str,
    expect_return_code: i32,
    working_dir: &'a Path,
}

fn run_test(config: &TestConfig) -> Result<(), TestError> {
    let output = Command::new("sh")
        .current_dir(config.working_dir)
        .args(["-c", config.command])
        .output()
        .expect("Failed to run mcc");

    let actual_code = output.status.code();

    if actual_code.is_none_or(|actual_code| actual_code != config.expect_return_code) {
        Err(TestError {
            expect_code: config.expect_return_code,
            actual_code,
            stderr: String::from_utf8(output.stderr).unwrap(),
        })
    } else {
        Ok(())
    }
}

#[derive(Deserialize, Debug)]
struct TestTomlConfig {
    command: String,

    #[serde(default)]
    return_code: i32,
}

#[derive(Deserialize, Debug)]
#[serde(untagged)]
enum TestTomlFile {
    Flat(TestTomlConfig),
    Nested {
        commands: HashMap<String, TestTomlConfig>,
    },
}

fn return_regex() -> &'static Regex {
    static RE: OnceLock<Regex> = OnceLock::new();
    RE.get_or_init(|| Regex::new(r"// +RETURN: +([0-9]+)$").unwrap())
}

fn print_test_case_message_prefix(path: &Path, suffix: &str) {
    let base_dir = &global_config().base_dir;

    print!("{}", path.strip_prefix(base_dir).unwrap().display());
    if !suffix.is_empty() {
        print!("[{}]", suffix);
    }
    print!(": ");
}

fn run_tests_with_command(
    states: &mut TestRunnerStates,
    folder: &Path,
    config: &TestTomlConfig,
    suffix: &str,
) {
    let global_config = global_config();
    let mcc_path = &global_config.mcc_path;

    let command = config
        .command
        .replace("{mcc}", &mcc_path.display().to_string());

    let return_re = return_regex();

    for entry in read_dir(folder).unwrap() {
        let path = entry.unwrap().path();

        if path.extension().is_some_and(|e| e == "c") {
            let command = command
                .replace("{filename}", &path.display().to_string())
                .replace("{base}", &path.with_extension("").display().to_string());

            let mut overrided_return = None;
            {
                let file = File::open(&path).unwrap();
                let reader = BufReader::new(file);

                for line in reader.lines() {
                    let line = line.unwrap();
                    if let Some(captures) = return_re.captures(&line) {
                        overrided_return = Some(captures.get(1).unwrap().as_str().parse().unwrap());
                    }
                }
            }

            let test_config = TestConfig {
                command: &command,
                expect_return_code: overrided_return.unwrap_or(config.return_code),
                working_dir: folder,
            };

            if let Err(error) = run_test(&test_config) {
                print_test_case_message_prefix(&path, suffix);
                states.failed_test_count += 1;
                println!("{}:\n{}", "Failed".red().bold(), error);
                println!("Command: {}", test_config.command);
                println!();
            } else if global_config.verbose {
                print_test_case_message_prefix(&path, suffix);
                println!("{}", "PASSED".green().bold());
            }

            // TODO: better way than this ad-hoc solution
            // Remove generated files
            let _ = std::fs::remove_file(path.with_extension("o"));
            let _ = std::fs::remove_file(path.with_extension("s"));
            let _ = std::fs::remove_file(path.with_extension(""));

            states.total_test_count += 1;
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

fn run_tests_in(states: &mut TestRunnerStates, current_folder: &Path) {
    for entry in read_dir(current_folder).unwrap() {
        let path = entry.unwrap().path();
        if path.is_dir() {
            run_tests_in(states, &path);
        }
    }

    let config_file = read_test_config_file(current_folder);
    if config_file.is_none() {
        return;
    }
    let config_file = config_file.unwrap();

    match config_file {
        TestTomlFile::Flat(config) => run_tests_with_command(states, &current_folder, &config, ""),
        TestTomlFile::Nested { commands } => {
            for (name, command) in &commands {
                run_tests_with_command(states, &current_folder, command, name)
            }
        }
    }
}

fn main() -> ExitCode {
    test_run_mcc();

    let mut states = TestRunnerStates::new();

    let base_folder = &global_config().base_dir;
    run_tests_in(&mut states, base_folder);

    let all_test_passes = states.failed_test_count == 0;

    let result_string = format!(
        "[{}/{}] tests pass",
        states.total_test_count - states.failed_test_count,
        states.total_test_count
    );
    if all_test_passes {
        println!("{}", result_string.green().bold());
        ExitCode::SUCCESS
    } else {
        println!("{}", result_string.red().bold());
        ExitCode::FAILURE
    }
}
