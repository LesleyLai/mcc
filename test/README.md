This project contains two types of tests. 
- Unit tests (located in `./unit_tests`)
- End-to-end tests (located in `./test_data`)


## End-to-End testing

To perform end-to-end testing, run the following commands. Please note that the test driver is implemented in Rust, so a Rust toolchain need to be installed.

```sh
cd <mcc project root>/test/test-driver
cargo -q run --release -- --mcc <mcc executable> --base-folder ../test_data
```

The test runner is heavily inspired by [turnt](https://pypi.org/project/turnt/).

### Test Configuration File

The test driver scans the base folder for subdirectories containing a `test_config.toml` file. The configuration file should follow this format:

```toml
command = "{mcc} {filename} ; {base}"
return_code = 1
```

#### `command`
The shell command to execute tests. The command is templated and the test runner support the following variables:
- `{mcc}`: The location of the mcc executable
- `{filename}`: The full path of the test file
- `{base}`: The path of the test file, with the extension removed

`command` is the only configuration option must given to test configuration.

#### `return_code`
By default, the test runner expect a return value of `0`. We can manually specify an expectedreturn value via `return_code`. 

### Override Test Configuration in Source

Test configurations can also be overridden directly in the source files. These overrides take precedence over the test_config.toml settings.

Currently, the only supported override is:
```c
// RETURN: 42
```
Which override the expected return code to something else.
