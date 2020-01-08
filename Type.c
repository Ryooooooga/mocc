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
