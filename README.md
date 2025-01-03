# Mini C Compiler (MCC)

Working-in-progress C compiler written in C. My goal is to make it able to self-host.

## Current Features

The mcc compiler follows a standard batch-compiler architecture, consisting of the following components:

- **Frontend**: lexers and parsers to transform C source file
  into [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree)
- **IR Generation**: Converts the AST into
  a [three-address code intermediate representation](https://en.wikipedia.org/wiki/Three-address_code).
- **Assembly Generation**: Translates the intermediate representation into x86-64 assembly code.

`mcc` invokes `gcc` for preprocessor, assembler, and linker. Only Linux is supported and tested.

At present, only a small subset of the C language is supported. You can find example programs demonstrating the
compiler’s capabilities in the [test/test_data](./test/test_data) directory. Additionally, mcc does not yet implement
any optimization and currently produces extremely inefficient code with no regard for performance.
