cmake_minimum_required(VERSION 3.20)
project(src C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

IF (WIN32)
    message(FATAL_ERROR "Windows is not supported. You can use WSL.")
ENDIF ()

# The "developer mode" is a one-stop setting that
# - enables address and UB sanitizers when possible
# - treats warning as errors
set(MCC_ENABLE_DEVELOPER_MODE FALSE CACHE BOOL "Enable 'developer mode'")

include("cmake/compiler.cmake")

add_subdirectory(src)

option(MCC_BUILD_TESTS "Build tests" ON)
if (MCC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
