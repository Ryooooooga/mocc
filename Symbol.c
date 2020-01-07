#include "mocc.h"

Symbol *Symbol_new(const char *name) {
    assert(name);

    Symbol *s = malloc(sizeof(Symbol));
    s->name = name;

    s->address = NULL;

    return s;
}