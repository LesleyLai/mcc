add_library(mcc_lib
        utils/str.h
        utils/arena.c
        utils/arena.h
        utils/defer.h
        utils/prelude.h
        utils/format.h
        utils/format.c
        utils/str.c

        lexer.h
        lexer.c
        ast.h
        parser.h
        parser.c
        source_location.h
        diagnostic.c
        diagnostic.h
        cli_args.c
        cli_args.h
        token.c
        token.h
        ast_printer.c)
target_link_libraries(mcc_lib
        PUBLIC mcc::compiler_options
        PRIVATE mcc::compiler_warnings)
target_include_directories(mcc_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(mcc main.c)
target_link_libraries(mcc PRIVATE mcc::compiler_options mcc_lib mcc::compiler_warnings)