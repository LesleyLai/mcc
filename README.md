# Mini C Compiler (MCC)

Working-in-progress C compiler written in C. My goal is to make it able to self-host.

## Status

`mcc` invokes `gcc` for preprocessor, assembler, and linker. Only Linux is supported and tested, and the only backend
available is x86-64.

At present, only a small subset of the C language is supported. You can find example programs demonstrating the
compiler’s capabilities in the [tests/test_data](./tests/test_data) directory. Additionally, mcc does not yet implement
any optimization and currently produces extremely inefficient code with no regard for performance.

## Build

```shell
cmake -S . -B ./build -DCMAKE_BUILD_TYPE:STRING=Release # config cmake
cmake --build ./build --config Release # build the project
```

Below are useful CMake flags to customize the build process:

| Flag                        | Meaning                                                                             |
|-----------------------------|-------------------------------------------------------------------------------------|
| `MCC_BUILD_TESTS`           | Builds unit tests                                                                   |
| `MCC_ENABLE_DEVELOPER_MODE` | Enables "developer mode", a one-stop setting to enable a bunch of safety features   |
| `MCC_WARNING_AS_ERROR`      | Treats warnings as errors. Automatically enabled by "developer mode"                |
| `MCC_USE_ASAN`              | Enable the Address Sanitizers. Automatically enabled by "developer mode"            |
| `MCC_USE_UBSAN`             | Enable the Undefined Behavior Sanitizers. Automatically enabled by "developer mode" |

## Execute

```c
./build/bin/mcc <path to a .c file>
```

## Tests

See [tests/README.md](tests/README.md) for more information.

## Internal

The mcc compiler follows a standard compiler architecture, consisting of the following components:

- **Frontend**: lexers and parsers to transform C source file
  into [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree)
- **IR Generation**: Converts the AST into
  a [three-address code intermediate representation](https://en.wikipedia.org/wiki/Three-address_code).
- **Assembly Generation**: Translates the intermediate representation into assembly code.
