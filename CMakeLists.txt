cmake_minimum_required(VERSION 3.20)
project(mcc C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("cmake/compiler.cmake")

add_subdirectory(mcc)

option(MCC_BUILD_TESTS "Build tests" ON)
if (MCC_BUILD_TESTS)
    include("cmake/conan.cmake")
    run_conan()

    enable_testing()
    add_subdirectory(test)
endif ()
