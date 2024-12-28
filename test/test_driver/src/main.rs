mod global_configuration;
mod test_configuration;
mod test_database;
mod tests_detector;

use colored::Colorize;
use std::fmt::Display;
use std::path::Path;
use std::process::{exit, Command, ExitCode};

use crate::global_configuration::global_config;
use crate::test_configuration::TestConfig;
use crate::test_database::TestDatabase;
use crate::tests_detector::detect_tests;

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

// Return true if test failed
async fn run_test(database: &TestDatabase, config: &TestConfig) -> Result<(), TestError> {
    let config = config.override_by_file(&database, config.path);

    let TestConfig {
        command,
        path,
        working_dir,
        expect_code,
        ..
    } = config;
    let path = database.get_path(path);

    let command = database
        .get_command(command)
        .replace("{filename}", &path.display().to_string())
        .replace("{base}", &path.with_extension("").display().to_string());

    let output = tokio::process::Command::new("sh")
        .current_dir(database.get_path(working_dir))
        .args(["-c", &command])
        .output()
        .await
        .expect("Failed to test command");

    let actual_code = output.status.code();

    // TODO: better way than this ad-hoc solution
    // Remove generated files
    use tokio::fs::remove_file;
    let _ = remove_file(path.with_extension("o")).await;
    let _ = remove_file(path.with_extension("i")).await;
    let _ = remove_file(path.with_extension("s")).await;
    let _ = remove_file(path.with_extension("")).await;

    if actual_code.is_none_or(|actual_code| actual_code != expect_code) {
        Err(TestError {
            expect_code,
            actual_code,
            stderr: String::from_utf8(output.stderr).unwrap(),
        })
    } else {
        Ok(())
    }
}

fn print_test_case_message_prefix(path: &Path, suffix: &str) {
    let base_dir = &global_config().base_dir;

    print!("{}", path.strip_prefix(base_dir).unwrap().display());
    if !suffix.is_empty() {
        print!("[{}]", suffix);
    }
    print!(": ");
}

async fn run_tests(database: &'static TestDatabase) -> ExitCode {
    let total_test_count = database.tests().len();
    let mut failed_test_count = 0;

    let start = std::time::Instant::now();
    let handles: Vec<_> = database
        .tests()
        .iter()
        .map(|config| tokio::spawn(run_test(&database, &config)))
        .collect::<Vec<_>>();

    for (i, handle) in handles.into_iter().enumerate() {
        // TODO: properly handle join error
        let config = database.tests()[i];
        let path = database.get_path(config.path);
        let suffix = database.get_string(config.suffix);

        if let Err(error) = handle.await.unwrap() {
            print_test_case_message_prefix(&path, suffix);
            println!("{}:\n{}", "Failed".red().bold(), error);
            println!("Command: {}", database.get_command(config.command));
            println!();

            failed_test_count += 1;
        } else {
            if !global_config().quiet {
                print_test_case_message_prefix(&path, suffix);
                println!("{}", "PASSED".green().bold());
            }
        }
    }

    let delta = std::time::Instant::now() - start;
    println!(
        "{} tests executed in: {:.4}s",
        database.tests().len(),
        delta.as_secs_f32()
    );

    let all_test_passes = failed_test_count == 0;

    let result_string = format!(
        "[{}/{}] tests pass",
        total_test_count - failed_test_count,
        total_test_count
    );
    if all_test_passes {
        println!("{}", result_string.green().bold());
        ExitCode::SUCCESS
    } else {
        println!("{}", result_string.red().bold());
        ExitCode::FAILURE
    }
}

fn main() -> ExitCode {
    test_run_mcc();

    let database = Box::leak(Box::new(detect_tests()));

    tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .build()
        .unwrap()
        .block_on(run_tests(database))
}
