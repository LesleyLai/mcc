#include <mcc/format.h>
#include <mcc/prelude.h>
#include <mcc/type.h>

const Type* typ_void = &(const Type){
    .tag = TYPE_VOID,
};

const Type* typ_int = (const Type*)&(const IntegerType){
    .base =
        {
            .tag = TYPE_INTEGER,
            .size = 4,
            .alignment = 4,
        },
    .is_unsigned = true,
    .name = "int",
};

const Type* func_type(const Type* return_type, uint32_t param_count,
                      Arena* arena)
{
  FunctionType* result = ARENA_ALLOC_OBJECT(arena, FunctionType);
  *result = (FunctionType){
      .base =
          {
              .tag = TYPE_FUNCTION,
              .size = 1,
              .alignment = 1,
          },
      .param_count = param_count,
      .return_type = return_type,
  };

  return (const Type*)result;
}

void format_type_to(StringBuffer* buffer, const Type* typ)
{
  switch (typ->tag) {
  case TYPE_VOID: string_buffer_append(buffer, str("void")); return;
  case TYPE_INTEGER: {
    const IntegerType* int_typ = (const IntegerType*)typ;
    string_buffer_append(buffer, str(int_typ->name));
  }
    return;
  case TYPE_FUNCTION: {
    const FunctionType* func_typ = (const FunctionType*)typ;
    format_type_to(buffer, func_typ->return_type);
    string_buffer_push(buffer, '(');
    if (func_typ->param_count != 0) {
      for (uint32_t i = 0; i < func_typ->param_count; ++i) {
        if (i != 0) { string_buffer_append(buffer, str(", ")); }
        string_buffer_append(buffer, str("int"));
      }
    } else {
      string_buffer_append(buffer, str("void"));
    }
    string_buffer_push(buffer, ')');
  }
    return;
  }
}
