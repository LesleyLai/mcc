
get_filename_component(include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../include/mcc ABSOLUTE)
add_library(mcc_lib
        ${include_dir}/str.h
        ${include_dir}/arena.h
        ${include_dir}/prelude.h
        ${include_dir}/format.h
        ${include_dir}/ast.h
        ${include_dir}/parser.h
        ${include_dir}/source_location.h
        ${include_dir}/diagnostic.h
        ${include_dir}/cli_args.h
        ${include_dir}/token.h
        ${include_dir}/ir.h
        ${include_dir}/x86.h

        utils/format.c
        utils/str.c
        utils/arena.c
        utils/arena_vm.c
        utils/diagnostic.c
        utils/cli_args.c

        frontend/lexer.c
        frontend/ast_printer.c
        frontend/parser.c
        frontend/token.c

        ir/ir_generator.c
        ir/ir_printer.c

        x86/x86_passes.h
        x86/x86.c
        x86/x86_helpers.h
        x86/x86_helpers.c
        x86/x86_printer.c
        x86/x86_fix_instructions.c
        x86/x86_replace_pseudos.c
        x86/x86_from_ir.c
)
target_link_libraries(mcc_lib
        PUBLIC mcc::compiler_options
        PRIVATE mcc::compiler_warnings)
target_include_directories(mcc_lib
        PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(mcc main.c)
target_link_libraries(mcc PRIVATE mcc::compiler_options mcc_lib mcc::compiler_warnings)
