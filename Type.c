#include "mocc.h"

static Type *Type_new(TypeKind kind) {
    Type *t = malloc(sizeof(Type));
    t->kind = kind;
    t->pointee_type = NULL;

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
