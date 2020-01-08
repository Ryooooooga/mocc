#include "mocc.h"

static Type *Type_new(TypeKind kind) {
    Type *t = malloc(sizeof(Type));
    t->kind = kind;

    return t;
}

Type *IntType_new(void) {
    return Type_new(TypeKind_int);
}
