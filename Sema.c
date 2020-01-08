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

// Implicit conversion
static ExprNode *Sema_implicit_cast(
    Sema *s,
    ValueCategory value_category,
    ImplicitCastOp
    operator,
    ExprNode *expression) {
    assert(s);
    assert(expression);

    (void)s;

    ImplicitCastExprNode *node =
        ImplicitCastExprNode_new(value_category, operator, expression);
    return ImplicitCastExprNode_base(node);
}

static void Sema_to_rvalue(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    if ((*expression)->value_category == ValueCategory_lvalue) {
        *expression = Sema_implicit_cast(
            s,
            ValueCategory_rvalue,
            ImplicitCastOp_lvalue_to_rvalue,
            *expression);
    }
}

static void Sema_integer_promotion(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    // TODO: integer promotion
    Sema_to_rvalue(s, expression);
}

static void
Sema_usual_arithmetic_conversion(Sema *s, ExprNode **lhs, ExprNode **rhs) {
    assert(s);
    assert(lhs);
    assert(*lhs);
    assert(rhs);
    assert(*rhs);

    // TODO: usual arithmetic conversion
    Sema_integer_promotion(s, lhs);
    Sema_integer_promotion(s, rhs);
}

static void Sema_assignment_conversion(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    // TODO: assignment conversion
    Sema_to_rvalue(s, expression);
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

    // TODO: enumerator
    IdentifierExprNode *node =
        IdentifierExprNode_new(ValueCategory_lvalue, symbol);
    return IdentifierExprNode_base(node);
}

ExprNode *Sema_act_on_integer_expr(Sema *s, const Token *integer) {
    assert(s);
    assert(integer);

    (void)s;

    long long value = atoll(integer->text); // TODO: conversion

    IntegerExprNode *node = IntegerExprNode_new(ValueCategory_rvalue, value);
    return IntegerExprNode_base(node);
}

ExprNode *Sema_act_on_binary_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs) {
    assert(s);
    assert(lhs);
    assert(operator);
    assert(rhs);

    (void)s;

    BinaryOp op;

    switch (operator->kind) {
    case '+':
        // Promotion
        Sema_usual_arithmetic_conversion(s, &lhs, &rhs);

        // TODO: Type check

        op = BinaryOp_add;
        break;

    case '-':
        // Promotion
        Sema_usual_arithmetic_conversion(s, &lhs, &rhs);

        // TODO: Type check

        op = BinaryOp_sub;
        break;

    default:
        ERROR("unknown binary operator %s\n", operator->text);
    }

    BinaryExprNode *node =
        BinaryExprNode_new(ValueCategory_rvalue, op, lhs, rhs);
    return BinaryExprNode_base(node);
}

ExprNode *Sema_act_on_assign_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs) {
    assert(s);
    assert(lhs);
    assert(operator);
    assert(rhs);

    (void)s;

    if (lhs->value_category != ValueCategory_lvalue) {
        ERROR("expression is not assignable\n");
    }

    switch (operator->kind) {
    case '=':
        // Conversion
        Sema_assignment_conversion(s, &rhs);

        // TODO: Type check
        break;

    default:
        ERROR("unknown assignment operator %s\n", operator->text);
    }

    // Assignments is rvalue in C
    ValueCategory value_category = ValueCategory_rvalue;

    AssignExprNode *node = AssignExprNode_new(value_category, lhs, rhs);
    return AssignExprNode_base(node);
}

// Statements
StmtNode *Sema_act_on_return_stmt(Sema *s, ExprNode *return_value) {
    assert(s);

    // TODO: Type check
    if (return_value != NULL) {
        // Conversion
        Sema_assignment_conversion(s, &return_value);
    }

    ReturnStmtNode *node = ReturnStmtNode_new(return_value);
    return ReturnStmtNode_base(node);
}

StmtNode *Sema_act_on_decl_stmt(Sema *s, Vec(DeclaratorNode) * declarators) {
    assert(declarators);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(declarators); i++) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);

        Vec_push(Symbol)(s->local_variables, symbol);
    }

    DeclStmtNode *node = DeclStmtNode_new(declarators);
    return DeclStmtNode_base(node);
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

    DirectDeclaratorNode *node = DirectDeclaratorNode_new(symbol);
    return DirectDeclaratorNode_base(node);
}

DeclaratorNode *Sema_act_on_init_declarator(
    Sema *s, DeclaratorNode *declarator, ExprNode *initializer) {
    assert(s);
    assert(declarator);
    assert(initializer);

    // Conversion
    Sema_assignment_conversion(s, &initializer);

    // TODO: Type check

    InitDeclaratorNode *node = InitDeclaratorNode_new(declarator, initializer);
    return InitDeclaratorNode_base(node);
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

    FunctionDeclNode *node =
        FunctionDeclNode_new(declarator, body, local_variables);
    return FunctionDeclNode_base(node);
}
