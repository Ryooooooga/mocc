#include "mocc.h"

Symbol *Symbol_new(const char *name, StorageClass storage_class, Type *type) {
    assert(name);
    assert(type);

    Symbol *s = malloc(sizeof(Symbol));
    s->name = name;
    s->storage_class = storage_class;
    s->type = type;

    s->enum_value = -1;
    s->has_body = false;
    s->address = NULL;

    return s;
}
