get_filename_component(include_dir ${CMAKE_CURRENT_SOURCE_DIR}/../include/mcc ABSOLUTE)
add_library(mcc_lib
        ${include_dir}/str.h
        ${include_dir}/arena.h
        ${include_dir}/prelude.h
        ${include_dir}/format.h
        ${include_dir}/ast.h
        ${include_dir}/frontend.h
        ${include_dir}/source_location.h
        ${include_dir}/diagnostic.h
        ${include_dir}/cli_args.h
        ${include_dir}/token.h
        ${include_dir}/ir.h
        ${include_dir}/x86.h
        ${include_dir}/dynarray.h
        ${include_dir}/type.h
        ${include_dir}/sema.h
        ${include_dir}/hash_table.h

        utils/format.c
        utils/str.c
        utils/arena.c
        utils/arena_vm.c
        utils/diagnostic.c
        utils/cli_args.c
        utils/hash_table.c

        frontend/line_numbers.c
        frontend/lexer.c
        frontend/ast_printer.c
        frontend/parser.c
        frontend/token.c
        frontend/type.c
        frontend/type_check.c
        frontend/symbol_table.h
        frontend/symbol_table.c

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
        x86/x86_symbols.h
        x86/x86_symbols.c
)
target_link_libraries(mcc_lib
        PUBLIC mcc::compiler_options
        PRIVATE mcc::compiler_warnings)
target_include_directories(mcc_lib
        PUBLIC ${PROJECT_SOURCE_DIR}/include
)

add_executable(mcc main.c)
target_link_libraries(mcc PRIVATE mcc::compiler_options mcc_lib mcc::compiler_warnings)
