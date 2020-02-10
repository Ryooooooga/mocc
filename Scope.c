#include "mocc.h"

struct Scope {
    Scope *parent_scope;
    Vec(Symbol) * symbols;
};

Scope *Scope_new(Scope *parent_scope) {
    Scope *s = malloc(sizeof(Scope));
    s->parent_scope = parent_scope;
    s->symbols = Vec_new(Symbol)();

    return s;
}

Scope *Scope_parent_scope(const Scope *s) {
    assert(s);

    return s->parent_scope;
}

Symbol *Scope_find(Scope *s, const char *name, bool recursive) {
    assert(s);
    assert(name);

    for (size_t i = 0; i < Vec_len(Symbol)(s->symbols); i = i + 1) {
        Symbol *symbol = Vec_get(Symbol)(s->symbols, i);
        if (strcmp(symbol->name, name) == 0) {
            return symbol;
        }
    }

    if (recursive && s->parent_scope) {
        return Scope_find(s->parent_scope, name, true);
    }

    return NULL;
}

bool Scope_try_register(Scope *s, Symbol *symbol) {
    assert(s);
    assert(symbol);
    assert(symbol->name);

    if (Scope_find(s, symbol->name, false)) {
        return false;
    }

    Vec_push(Symbol)(s->symbols, symbol);
    return true;
}
