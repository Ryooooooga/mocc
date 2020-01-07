#include "mocc.h"

struct Sema {
    Scope *current_scope;
    Vec(Symbol) * local_variables;
};

Sema *Sema_new(void) {
    Sema *s = malloc(sizeof(Sema));
    s->current_scope = Scope_new(NULL);
    s->local_variables = NULL;

    return s;
}

static void Sema_push_scope_stack(Sema *s) {
    assert(s);

    s->current_scope = Scope_new(s->current_scope);
}

static void Sema_pop_scope_stack(Sema *s) {
    assert(s);
    assert(s->current_scope);

    s->current_scope = Scope_parent_scope(s->current_scope);
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

// Statements
StmtNode *Sema_act_on_decl_stmt(Sema *s, Vec(DeclaratorNode) * declarators) {
    assert(declarators);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(declarators); i++) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);

        Vec_push(Symbol)(s->local_variables, symbol);
    }

    DeclStmtNode *stmt = DeclStmtNode_new(declarators);
    return DeclStmtNode_base(stmt);
}

// Declarators
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

// Declarations
void Sema_act_on_function_decl_start_of_body(
    Sema *s, DeclaratorNode *declarator) {
    assert(s);
    assert(declarator);

    // Enter the function scope
    Sema_push_scope_stack(s);

    s->local_variables = Vec_new(Symbol)();

    // TODO: Process parameters
    (void)declarator;
}

DeclNode *Sema_act_on_function_decl_end_of_body(
    Sema *s, DeclaratorNode *declarator, StmtNode *body) {
    assert(s);
    assert(declarator);
    assert(body);

    // Leave the function scope
    Sema_pop_scope_stack(s);

    Vec(Symbol) *local_variables = s->local_variables;
    s->local_variables = NULL;

    FunctionDeclNode *decl =
        FunctionDeclNode_new(declarator, body, local_variables);
    return FunctionDeclNode_base(decl);
}
