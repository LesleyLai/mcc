use crate::global_configuration::global_config;
use crate::snapshot_testing::{snapshot_match, SnapshotError};
use crate::test_configuration::TestConfig;
use crate::test_database::TestDatabase;
use std::fmt::Display;
use std::sync::Arc;
use std::time::Duration;

pub struct TestError {
    expect_code: i32,
    actual_code: Option<i32>, // If no actual code, that means the code is returned as a signal
    actual_stderr: Arc<str>,
    pub stderr_snapshot_result: Result<(), SnapshotError>, // only have something when we have a mismatch in actual stderr and expected stderr
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
                Some(code) => writeln!(f, "{}", code)?,
                None => writeln!(f, "none")?,
            }
        }

        if let Err(error) = &self.stderr_snapshot_result {
            writeln!(f, "Standard error is different than expected:\n{}", error)?;
        } else if !self.actual_stderr.is_empty() {
            writeln!(f, "with error message:\n{}", &self.actual_stderr)?;
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

    let actual_stderr = String::from_utf8_lossy(&output.stderr);
    let actual_stderr =
        actual_stderr.replace(global_config().base_dir.to_str().unwrap(), "{{base_dir}}");
    let actual_stderr: Arc<str> = Arc::from(actual_stderr);

    let mut stderr_snapshot_result = Ok(());
    if config.snapshot_test_stderr {
        let std_error_approved_path = path.with_extension("stderr.approved.txt");

        stderr_snapshot_result =
            snapshot_match(actual_stderr.clone(), &std_error_approved_path).await;
    }

    // TODO: better way than this ad-hoc solution
    // Remove generated files
    use tokio::fs::remove_file;
    let _ = remove_file(path.with_extension("o")).await;
    let _ = remove_file(path.with_extension("i")).await;
    let _ = remove_file(path.with_extension("s")).await;
    let _ = remove_file(path.with_extension("")).await;

    if actual_code.is_none_or(|actual_code| actual_code != expect_code)
        || stderr_snapshot_result.is_err()
    {
        Err(TestError {
            expect_code,
            actual_code,
            actual_stderr,
            stderr_snapshot_result,
        })
    } else {
        Ok(())
    }
}

pub struct TestsOutput {
    pub results: Vec<Result<(), TestError>>,
    pub time: Duration,
}

pub async fn run_tests(database: &'static TestDatabase) -> TestsOutput {
    let start = std::time::Instant::now();
    let handles: Vec<_> = database
        .tests()
        .iter()
        .map(|config| tokio::spawn(run_test(&database, &config)))
        .collect::<Vec<_>>();

    // TODO: properly handle join error
    let results = futures::future::try_join_all(handles).await.unwrap();

    let delta = std::time::Instant::now() - start;

    TestsOutput {
        results,
        time: delta,
    }
}
