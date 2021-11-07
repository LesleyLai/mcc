cmake_minimum_required(VERSION 3.20)
project(mcc C)

set(CMAKE_C_STANDARD 11)

add_executable(mcc main.c lexer.h lexer.c string_view.h)
