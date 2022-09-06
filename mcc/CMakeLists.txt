add_library(mcc_lib
        lexer.h
        lexer.c
        utils/str.h
        utils/allocators.c
        utils/allocators.h
        ast.h
        parser.h
        parser.c utils/str.c source_location.h utils/defer.h utils/prelude.h utils/format.h utils/format.c)
target_link_libraries(mcc_lib
        PUBLIC mcc::compiler_options
        PRIVATE mcc::compiler_warnings)
target_include_directories(mcc_lib
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        )

add_executable(mcc main.c)
target_link_libraries(mcc PRIVATE mcc_lib mcc::compiler_warnings)