add_library(mcc_compiler_warnings INTERFACE)
add_library(mcc::compiler_warnings ALIAS mcc_compiler_warnings)

add_library(mcc_compiler_options INTERFACE)
add_library(mcc::compiler_options ALIAS mcc_compiler_options)


# Compiler specific settings

option(MCC_WARNING_AS_ERROR "Treats compiler warnings as errors" ON)
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
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2)
    if (MCC_WARNING_AS_ERROR)
        target_compile_options(mcc_compiler_warnings INTERFACE -Werror)
    endif ()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(mcc_compiler_warnings
                INTERFACE -Wmisleading-indentation
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                )
    endif ()
endif ()

option(MCC_USE_ASAN "Enable the Address Sanitizers" OFF)
if (MCC_USE_ASAN)
    message("Enable Address Sanitizer")
    target_compile_options(mcc_compiler_options INTERFACE
            -fsanitize=address -fno-omit-frame-pointer)
    target_link_libraries(mcc_compiler_options INTERFACE
            -fsanitize=address)
endif ()

option(MCC_USE_TSAN "Enable the Thread Sanitizers" OFF)
if (MCC_USE_TSAN)
    message("Enable Thread Sanitizer")
    target_compile_options(mcc_compiler_options INTERFACE
            -fsanitize=thread)
    target_link_libraries(mcc_compiler_options INTERFACE
            -fsanitize=thread)
endif ()

option(MCC_USE_MSAN "Enable the Memory Sanitizers" OFF)
if (MCC_USE_MSAN)
    message("Enable Memory Sanitizer")
    target_compile_options(mcc_compiler_options INTERFACE
            -fsanitize=memory -fno-omit-frame-pointer)
    target_link_libraries(mcc_compiler_options INTERFACE
            -fsanitize=memory)
endif ()

option(MCC_USE_UBSAN "Enable the Undefined Behavior Sanitizers" OFF)
if (MCC_USE_UBSAN)
    message("Enable Undefined Behavior Sanitizer ")
    target_compile_options(mcc_compiler_options INTERFACE
            -fsanitize=undefined)
    target_link_libraries(mcc_compiler_options INTERFACE
            -fsanitize=undefined)
endif ()