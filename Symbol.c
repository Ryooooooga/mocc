#include "mocc.h"

Symbol *Symbol_new(const char *name, Type *type) {
    assert(name);

    Symbol *s = malloc(sizeof(Symbol));
    s->name = name;
    s->type = type;

    s->address = NULL;

    return s;
}
