use clap::Parser;
use std::path::PathBuf;
use std::sync::OnceLock;

pub struct TestRunnerConfig {
    pub mcc_path: PathBuf, // Path of MCC executable
    pub base_dir: PathBuf, // Path to the base folder of all test files
    pub quiet: bool,
    pub interactive: bool,
}

pub fn global_config() -> &'static TestRunnerConfig {
    static CONFIG: OnceLock<TestRunnerConfig> = OnceLock::new();
    CONFIG.get_or_init(|| {
        let args = Args::parse();

        let mcc_path = args
            .mcc
            .canonicalize()
            .expect("Can't canonicalize mcc path");
        let base_dir = args
            .base_folder
            .canonicalize()
            .expect("Can't canonicalize base directory path");

        TestRunnerConfig {
            mcc_path,
            base_dir,
            quiet: args.quiet,
            interactive: args.interactive,
        }
    })
}

#[derive(Parser, Debug)]
#[command(about, long_about = None)]
struct Args {
    /// Path of MCC executable
    #[arg(long)]
    mcc: PathBuf,

    /// Path to the base folder of all test files
    #[arg(long)]
    base_folder: PathBuf,

    /// Suppress any output except for test failures.
    #[arg(short, long, default_value_t = false)]
    quiet: bool,

    /// If the snapshot test fails, prompt the user to decide whether to update the approved file.
    #[arg(short, long, default_value_t = false)]
    interactive: bool,
}
