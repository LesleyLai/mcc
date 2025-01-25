mod global_configuration;
mod snapshot_testing;
mod test_configuration;
mod test_database;
mod test_detector;
mod test_reporter;
mod test_runner;

use std::process::{exit, Command, ExitCode};

use crate::{
    global_configuration::global_config, test_detector::detect_tests, test_reporter::report_tests,
    test_runner::run_tests,
};

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

fn main() -> ExitCode {
    test_run_mcc();

    let database = Box::leak(Box::new(detect_tests()));

    let test_output = tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .build()
        .unwrap()
        .block_on(run_tests(database));

    report_tests(&database, test_output)
}
