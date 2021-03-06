#include "mocc.h"

static Type *Type_new(TypeKind kind) {
    Type *t = malloc(sizeof(Type));
    t->kind = kind;
    t->pointee_type = NULL;
    t->return_type = NULL;
    t->parameter_types = NULL;
    t->element_type = NULL;
    t->array_length = 0;
    t->struct_symbol = NULL;
    t->member_decls = NULL;
    t->member_symbols = NULL;
    t->underlying_type = NULL;
    t->enum_symbol = NULL;
    t->enumerators = NULL;

    return t;
}

Type *VoidType_new(void) {
    return Type_new(TypeKind_void);
}

Type *CharType_new(void) {
    return Type_new(TypeKind_char);
}

Type *IntType_new(void) {
    return Type_new(TypeKind_int);
}

Type *PointerType_new(Type *pointee_type) {
    assert(pointee_type);

    Type *type = Type_new(TypeKind_pointer);
    type->pointee_type = pointee_type;

    return type;
}

Type *PointerType_pointee_type(const Type *pointer_type) {
    assert(pointer_type);
    assert(pointer_type->kind == TypeKind_pointer);
    assert(pointer_type->pointee_type);

    return pointer_type->pointee_type;
}

Type *FunctionType_new(
    Type *return_type, Vec(Type) * parameter_types, bool is_var_arg) {
    assert(return_type);
    assert(parameter_types);

    Type *type = Type_new(TypeKind_function);
    type->return_type = return_type;
    type->parameter_types = parameter_types;
    type->is_var_arg = is_var_arg;

    return type;
}

Type *FunctionType_return_type(const Type *function_type) {
    assert(function_type);
    assert(function_type->kind == TypeKind_function);
    assert(function_type->return_type);

    return function_type->return_type;
}

Vec(Type) * FunctionType_parameter_types(const Type *function_type) {
    assert(function_type);
    assert(function_type->kind == TypeKind_function);
    assert(function_type->parameter_types);

    return function_type->parameter_types;
}

bool FunctionType_is_var_arg(const Type *function_type) {
    assert(function_type);
    assert(function_type->kind == TypeKind_function);

    return function_type->is_var_arg;
}

Type *ArrayType_new(Type *element_type, size_t array_length) {
    assert(element_type);
    assert(array_length > 0);

    Type *type = Type_new(TypeKind_array);
    type->element_type = element_type;
    type->array_length = array_length;

    return type;
}

Type *ArrayType_element_type(const Type *array_type) {
    assert(array_type);
    assert(array_type->kind == TypeKind_array);
    assert(array_type->element_type);

    return array_type->element_type;
}

size_t ArrayType_length(const Type *array_type) {
    assert(array_type);
    assert(array_type->kind == TypeKind_array);

    return array_type->array_length;
}

Type *StructType_new(void) {
    Type *type = Type_new(TypeKind_struct);

    return type;
}

bool StructType_is_defined(const Type *struct_type) {
    assert(struct_type);
    assert(struct_type->kind == TypeKind_struct);

    return struct_type->member_symbols != (Vec(Symbol) *)NULL;
}

struct Symbol *
StructType_find_member(const Type *struct_type, const char *member_name) {
    assert(struct_type);
    assert(struct_type->kind == TypeKind_struct);
    assert(StructType_is_defined(struct_type));
    assert(member_name);

    assert(struct_type->member_symbols);

    for (size_t i = 0; i < Vec_len(Symbol)(struct_type->member_symbols);
         i = i + 1) {
        Symbol *symbol = Vec_get(Symbol)(struct_type->member_symbols, i);
        assert(symbol->name);

        if (strcmp(symbol->name, member_name) == 0) {
            return symbol;
        }
    }

    return NULL;
}

Type *
EnumType_new(Type *underlying_type, Vec(EnumeratorDeclNode) * enumerators) {
    assert(underlying_type);
    assert(enumerators);

    Type *type = Type_new(TypeKind_enum);
    type->underlying_type = underlying_type;
    type->enum_symbol = NULL;
    type->enumerators = enumerators;

    return type;
}

size_t Type_sizeof(const Type *type) {
    assert(type);
    assert(!Type_is_incomplete_type(type));

    if (type->kind == TypeKind_char) {
        return 1;
    } else if (type->kind == TypeKind_int) {
        return 4;
    } else if (type->kind == TypeKind_pointer) {
        return 8;
    } else if (type->kind == TypeKind_array) {
        return Type_sizeof(ArrayType_element_type(type)) *
               ArrayType_length(type);
    } else if (type->kind == TypeKind_struct) {
        size_t size = 0;

        for (size_t i = 0; i < Vec_len(Symbol)(type->member_symbols);
             i = i + 1) {
            Symbol *symbol = Vec_get(Symbol)(type->member_symbols, i);
            size_t member_size = Type_sizeof(symbol->type);
            size_t member_align = Type_alignof(symbol->type);
            size_t padding = 0;

            if (size % member_align != 0) {
                padding = member_align - size % member_align;
            }

            size = size + padding + member_size;
        }

        return size;
    } else if (type->kind == TypeKind_enum) {
        return Type_sizeof(type->underlying_type);
    } else {
        UNREACHABLE();
    }
}

size_t Type_alignof(const Type *type) {
    assert(type);
    assert(!Type_is_incomplete_type(type));

    if (type->kind == TypeKind_char) {
        return 1;
    } else if (type->kind == TypeKind_int) {
        return 4;
    } else if (type->kind == TypeKind_pointer) {
        return 8;
    } else if (type->kind == TypeKind_array) {
        return Type_alignof(ArrayType_element_type(type));
    } else if (type->kind == TypeKind_struct) {
        size_t align = 0;

        for (size_t i = 0; i < Vec_len(Symbol)(type->member_symbols);
             i = i + 1) {
            Symbol *symbol = Vec_get(Symbol)(type->member_symbols, i);
            size_t member_align = Type_alignof(symbol->type);

            if (member_align > align) {
                align = member_align;
            }
        }

        return align;
    } else if (type->kind == TypeKind_enum) {
        return Type_alignof(type->underlying_type);
    } else {
        UNREACHABLE();
    }
}

bool Type_equals(const Type *a, const Type *b) {
    assert(a);
    assert(b);

    if (a == b) {
        return true;
    }

    if (a->kind != b->kind) {
        return false;
    }

    if (a->kind == TypeKind_void || a->kind == TypeKind_char ||
        a->kind == TypeKind_int) {
        return true;
    } else if (a->kind == TypeKind_pointer) {
        return Type_equals(
            PointerType_pointee_type(a), PointerType_pointee_type(b));
    } else if (a->kind == TypeKind_function) {
        const Vec(Type) *a_params = FunctionType_parameter_types(a);
        const Vec(Type) *b_params = FunctionType_parameter_types(b);

        if (a->is_var_arg != b->is_var_arg) {
            return false;
        }

        if (Vec_len(Type)(a_params) != Vec_len(Type)(b_params)) {
            return false;
        }

        for (size_t i = 0; i < Vec_len(Type)(a_params); i = i + 1) {
            const Type *a_param = Vec_get(Type)(a_params, i);
            const Type *b_param = Vec_get(Type)(b_params, i);

            if (!Type_equals(a_param, b_param)) {
                return false;
            }
        }

        return Type_equals(
            FunctionType_return_type(a), FunctionType_return_type(b));
    } else if (a->kind == TypeKind_array) {
        return ArrayType_length(a) == ArrayType_length(b) &&
               Type_equals(
                   ArrayType_element_type(a), ArrayType_element_type(b));
    } else if (a->kind == TypeKind_struct || a->kind == TypeKind_enum) {
        return false;
    } else {
        UNREACHABLE();
    }
}

bool Type_is_scalar(const Type *type) {
    assert(type);

    return type->kind == TypeKind_char || type->kind == TypeKind_int ||
           type->kind == TypeKind_pointer;
}

bool Type_is_incomplete_type(const Type *type) {
    assert(type);

    return type->kind == TypeKind_void || type->kind == TypeKind_function;
}

bool Type_is_function_pointer_type(const Type *type) {
    assert(type);

    return type->kind == TypeKind_pointer &&
           PointerType_pointee_type(type)->kind == TypeKind_function;
}
