add_library(mcc_compiler_warnings INTERFACE)
add_library(mcc::compiler_warnings ALIAS mcc_compiler_warnings)

add_library(mcc_compiler_options INTERFACE)
add_library(mcc::compiler_options ALIAS mcc_compiler_options)

include(CMakeDependentOption)
cmake_dependent_option(MCC_WARNING_AS_ERROR "Treats compiler warnings as errors" ON MCC_ENABLE_DEVELOPER_MODE OFF)
cmake_dependent_option(MCC_USE_ASAN "Enable the Address Sanitizers" ON MCC_ENABLE_DEVELOPER_MODE OFF)
cmake_dependent_option(MCC_USE_UBSAN "Enable the Undefined Behavior Sanitizers" ON MCC_ENABLE_DEVELOPER_MODE OFF)

# Compiler specific settings
if (MSVC)
    target_compile_options(mcc_compiler_warnings
            INTERFACE
            /W3
            " /permissive-"
            " /wd4146" # C4146: unary minus operator applied to unsigned type, result still unsigned
    )
    target_compile_definitions(mcc_compiler_warnings INTERFACE _CRT_SECURE_NO_DEPRECATE)
    if (MCC_WARNING_AS_ERROR)
        target_compile_options(mcc_compiler_warnings INTERFACE " /WX")
    endif ()
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mcc_compiler_warnings
            INTERFACE -Wall
            -Wextra
            -Wshadow
            -Wcast-align
            -Wunused
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
            $<$<COMPILE_LANGUAGE:CXX>:
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            >
    )
    if (MCC_WARNING_AS_ERROR)
        target_compile_options(mcc_compiler_warnings INTERFACE -Werror)
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mcc_compiler_warnings
                INTERFACE
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wduplicated-branches
                -Wno-format-truncation
                -Wlogical-op
        )
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        target_compile_options(mcc_compiler_warnings
                INTERFACE
                -Wno-keyword-macro
        )
    endif ()
endif ()

if (MCC_USE_ASAN)
    message("Enable Address Sanitizer")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(mcc_compiler_options INTERFACE
                -fsanitize=address -fno-omit-frame-pointer)
        target_link_libraries(mcc_compiler_options INTERFACE
                -fsanitize=address)
    endif ()
endif ()

if (MCC_USE_UBSAN)
    message("Enable Undefined Behavior Sanitizer ")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(mcc_compiler_options INTERFACE
                -fsanitize=undefined)
        target_link_libraries(mcc_compiler_options INTERFACE
                -fsanitize=undefined)
    endif ()
endif ()
