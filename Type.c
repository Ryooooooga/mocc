#include "mocc.h"

static Type *Type_new(TypeKind kind) {
    Type *t = malloc(sizeof(Type));
    t->kind = kind;
    t->pointee_type = NULL;
    t->return_type = NULL;

    return t;
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
    assert(pointer_type->pointee_type != NULL);

    return pointer_type->pointee_type;
}

Type *FunctionType_new(Type *return_type, Vec(Type) * parameter_types) {
    assert(return_type);
    assert(parameter_types);

    Type *type = Type_new(TypeKind_function);
    type->return_type = return_type;
    type->parameter_types = parameter_types;

    return type;
}

Type *FunctionType_return_type(const Type *function_type) {
    assert(function_type);
    assert(function_type->kind == TypeKind_function);
    assert(function_type->return_type != NULL);

    return function_type->return_type;
}

Vec(Type) * FunctionType_parameter_types(const Type *function_type) {
    assert(function_type);
    assert(function_type->kind == TypeKind_function);
    assert(function_type->parameter_types != NULL);

    return function_type->parameter_types;
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

size_t Type_sizeof(const Type *type) {
    assert(type);

    switch (type->kind) {
    case TypeKind_void:
    case TypeKind_function:
        return 0;
    case TypeKind_int:
        return 4;
    case TypeKind_pointer:
        return 8;
    case TypeKind_array:
        return Type_sizeof(ArrayType_element_type(type)) *
               ArrayType_length(type);
    default:
        UNREACHABLE();
    }
}

size_t Type_alignof(const Type *type) {
    assert(type);

    switch (type->kind) {
    case TypeKind_void:
    case TypeKind_function:
        return 0;
    case TypeKind_int:
        return 4;
    case TypeKind_pointer:
        return 8;
    case TypeKind_array:
        return Type_alignof(ArrayType_element_type(type));
    default:
        UNREACHABLE();
    }
}

bool Type_is_incomplete_type(const Type *type) {
    assert(type);

    switch (type->kind) {
    case TypeKind_void:
    case TypeKind_function:
        return true;

    default:
        return false;
    }
}

bool Type_is_function_pointer_type(const Type *type) {
    assert(type);

    return type->kind == TypeKind_pointer &&
           PointerType_pointee_type(type)->kind == TypeKind_function;
}
