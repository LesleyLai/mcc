# Mini C Compiler (MCC)

Working-in-progress C compiler written in C. My goal is to make it able to self-host.

## Build

### Requirement

Conan package manager is required for tools and tests. It is not required for the mcc compiler itself (which has no
dependencies).

## Run

### Requirement

YASM installed and is in the path

Windows Specific setup:

- `link.exe` need to be in the path C:\Program Files (x86)\Microsoft Visual
  Studio\2019\Community\VC\Tools\MSVC\14.29.30133\bin\Hostx64\x64