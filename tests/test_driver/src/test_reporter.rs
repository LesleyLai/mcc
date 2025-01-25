use crate::test_runner::TestsOutput;
use crate::{global_configuration::global_config, test_database::TestDatabase};
use colored::Colorize;
use std::{
    io::{stdout, Write},
    path::Path,
    process::ExitCode,
};

fn print_test_case_message_prefix(path: &Path, suffix: &str) {
    let base_dir = &global_config().base_dir;

    print!("{}", path.strip_prefix(base_dir).unwrap().display());
    if !suffix.is_empty() {
        print!("[{}]", suffix);
    }
    print!(": ");
}

fn yes_or_no_input(prompt: &str) -> bool {
    let stdin = std::io::stdin();

    loop {
        print!("{}", prompt);
        stdout().flush().unwrap();
        let mut buffer = String::new();
        stdin.read_line(&mut buffer).unwrap();

        match buffer.trim_end() {
            "y" | "yes" => return true,
            "n" | "no" => return false,
            _ => {}
        }
    }
}

pub fn report_tests(database: &TestDatabase, test_output: TestsOutput) -> ExitCode {
    let total_test_count = database.tests().len();
    let mut failed_test_count = 0;

    for (i, result) in test_output.results.into_iter().enumerate() {
        let config = database.tests()[i];
        let path = database.get_path(config.path);
        let suffix = database.get_string(config.suffix);

        if let Err(error) = result {
            print_test_case_message_prefix(&path, suffix);
            println!("{}:", "Failed".red().bold());
            println!("{error}");

            if let Err(snapshot_error) = &error.stderr_snapshot_result {
                if global_config().interactive
                    && yes_or_no_input("overwrite approved file [yes/no]? ")
                {
                    std::fs::write(
                        &snapshot_error.expected_path,
                        snapshot_error.actual.as_ref(),
                    )
                    .unwrap();
                }
            }

            failed_test_count += 1;
        } else {
            if !global_config().quiet {
                print_test_case_message_prefix(&path, suffix);
                println!("{}", "PASSED".green().bold());
            }
        }
    }

    let all_test_passes = failed_test_count == 0;

    println!(
        "{} tests executed in: {:.4}s",
        database.tests().len(),
        test_output.time.as_secs_f32()
    );

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
