add_library(mcc_lib
        lexer.h
        lexer.c
        string_view.h
        arena.c
        arena.h
        ast.h
        parser.h
        parser.c)
target_include_directories(mcc_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mcc main.c)
target_link_libraries(mcc PRIVATE mcc_lib)