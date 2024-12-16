use clap::Parser;
use std::path::PathBuf;
use std::sync::OnceLock;

pub struct TestRunnerConfig {
    pub mcc_path: PathBuf, // Path of MCC executable
    pub base_dir: PathBuf, // Path to the base folder of all test files
    pub verbose: bool,
}

pub fn global_config() -> &'static TestRunnerConfig {
    static CONFIG: OnceLock<TestRunnerConfig> = OnceLock::new();
    CONFIG.get_or_init(|| {
        let args = Args::parse();

        let mcc_path = args.mcc.canonicalize().unwrap();
        let base_dir = args.base_folder.canonicalize().unwrap();
        let verbose = args.verbose;

        TestRunnerConfig {
            mcc_path,
            base_dir,
            verbose,
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

    /// Print more messages, such as passed tests
    #[arg(short, long, default_value_t = false)]
    verbose: bool,
}
