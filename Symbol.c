#include "mocc.h"

Symbol *Symbol_new(const char *name, Type *type) {
    assert(name);
    assert(type);

    Symbol *s = malloc(sizeof(Symbol));
    s->name = name;
    s->type = type;

    s->has_body = false;
    s->address = NULL;

    return s;
}
