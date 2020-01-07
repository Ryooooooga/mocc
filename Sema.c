#include "mocc.h"

struct Sema {
    Scope *current_scope;
};

Sema *Sema_new(void) {
    Sema *s = malloc(sizeof(Sema));
    s->current_scope = Scope_new(NULL);

    return s;
}

static Symbol *Sema_find_symbol(Sema *s, const char *name) {
    assert(s);
    assert(s->current_scope);
    assert(name);

    return Scope_find(s->current_scope, name, true);
}

static bool Sema_try_register_symbol(Sema *s, Symbol *symbol) {
    assert(s);
    assert(s->current_scope);
    assert(symbol);

    return Scope_try_register(s->current_scope, symbol);
}

// Expressions
ExprNode *Sema_act_on_identifier_expr(Sema *s, const Token *identifier) {
    assert(s);
    assert(identifier);

    // Look up the symbol from context
    Symbol *symbol = Sema_find_symbol(s, identifier->text);
    if (symbol == NULL) {
        ERROR("undeclared identifier %s\n", identifier->text);
    }

    IdentifierExprNode *expr = IdentifierExprNode_new(symbol);
    return IdentifierExprNode_base(expr);
}

// Declarations
DeclaratorNode *
Sema_act_on_direct_declarator(Sema *s, const Token *identifier) {
    assert(s);
    assert(identifier);

    // Register the symbol
    Symbol *symbol = Symbol_new(identifier->text);
    if (!Sema_try_register_symbol(s, symbol)) {
        ERROR("%s is already declared in this scope\n", identifier->text);
    }

    DirectDeclaratorNode *declarator = DirectDeclaratorNode_new(symbol);
    return DirectDeclaratorNode_base(declarator);
}
