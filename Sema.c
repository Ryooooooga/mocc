#include "mocc.h"

struct Sema {
    Scope *current_variable_scope;
    Scope *current_struct_scope;
    Vec(Symbol) * local_variables;
    Type *return_type;
    Type *char_type;
    Type *int_type;
};

Sema *Sema_new(void) {
    Sema *s = malloc(sizeof(Sema));
    s->current_variable_scope = Scope_new(NULL);
    s->current_struct_scope = Scope_new(NULL);
    s->local_variables = NULL;
    s->return_type = NULL;
    s->char_type = CharType_new();
    s->int_type = IntType_new();

    return s;
}

static void Sema_push_scope_stack(Sema *s) {
    assert(s);

    s->current_variable_scope = Scope_new(s->current_variable_scope);
    s->current_struct_scope = Scope_new(s->current_struct_scope);
}

static void Sema_pop_scope_stack(Sema *s) {
    assert(s);
    assert(s->current_variable_scope);
    assert(s->current_struct_scope);

    s->current_variable_scope = Scope_parent_scope(s->current_variable_scope);
    s->current_struct_scope = Scope_parent_scope(s->current_struct_scope);
}

static void Sema_push_scope_stack_members(Sema *s) {
    assert(s);

    s->current_struct_scope = Scope_new(s->current_struct_scope);
}

static void Sema_pop_scope_stack_members(Sema *s) {
    assert(s);
    assert(s->current_struct_scope);

    s->current_struct_scope = Scope_parent_scope(s->current_struct_scope);
}

static Symbol *Sema_find_symbol(Sema *s, const char *name) {
    assert(s);
    assert(s->current_variable_scope);
    assert(name);

    return Scope_find(s->current_variable_scope, name, true);
}

static void Sema_register_symbol(Sema *s, Symbol *symbol) {
    assert(s);
    assert(s->current_variable_scope);
    assert(symbol);

    if (!Scope_try_register(s->current_variable_scope, symbol)) {
        ERROR("%s is already declared in this scope\n", symbol->name);
    }
}

static Symbol *Sema_find_struct_symbol(Sema *s, const char *name) {
    assert(s);
    assert(s->current_struct_scope);
    assert(name);

    return Scope_find(s->current_struct_scope, name, true);
}

static void Sema_register_struct_symbol(Sema *s, Symbol *symbol) {
    assert(s);
    assert(s->current_struct_scope);
    assert(symbol);

    if (!Scope_try_register(s->current_struct_scope, symbol)) {
        ERROR("struct %s is already declared in this scope\n", symbol->name);
    }
}

// Implicit conversion
static ExprNode *Sema_implicit_cast(
    Sema *s,
    Type *destination_type,
    ValueCategory value_category,
    ImplicitCastOp
    operator,
    ExprNode *expression) {
    assert(s);
    assert(destination_type);
    assert(expression);

    (void)s;

    ImplicitCastExprNode *node = ImplicitCastExprNode_new(
        destination_type, value_category, operator, expression);
    return ImplicitCastExprNode_base(node);
}

static void Sema_to_rvalue(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    if ((*expression)->value_category == ValueCategory_lvalue) {
        *expression = Sema_implicit_cast(
            s,
            (*expression)->result_type,
            ValueCategory_rvalue,
            ImplicitCastOp_lvalue_to_rvalue,
            *expression);
    }
}

static void Sema_decay_conversion(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    if ((*expression)->result_type->kind == TypeKind_function) {
        // function to pointer conversion
        Type *function_type = (*expression)->result_type;

        *expression = Sema_implicit_cast(
            s,
            PointerType_new(function_type),
            ValueCategory_rvalue,
            ImplicitCastOp_function_to_function_pointer,
            (*expression));
    } else if ((*expression)->result_type->kind == TypeKind_array) {
        // array to pointer conversion
        Type *element_type = ArrayType_element_type((*expression)->result_type);

        *expression = Sema_implicit_cast(
            s,
            PointerType_new(element_type),
            ValueCategory_rvalue,
            ImplicitCastOp_array_to_pointer,
            (*expression));
    } else {
        Sema_to_rvalue(s, expression);
    }
}

static void Sema_integer_promotion(Sema *s, ExprNode **expression) {
    assert(s);
    assert(expression);
    assert(*expression);

    Sema_decay_conversion(s, expression);

    switch ((*expression)->result_type->kind) {
    case TypeKind_char:
        // char or short -> int
        *expression = Sema_implicit_cast(
            s,
            s->int_type,
            ValueCategory_rvalue,
            ImplicitCastOp_integral_cast,
            *expression);
        break;

    default:
        break;
    }
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

static void Sema_assignment_conversion(
    Sema *s, Type *destination_type, ExprNode **expression) {
    assert(s);
    assert(destination_type);
    assert(expression);
    assert(*expression);

    Sema_decay_conversion(s, expression);
    Type *from_type = (*expression)->result_type;

    if (Type_equals(from_type, destination_type)) {
        return;
    }

    switch (destination_type->kind) {
    case TypeKind_char:
    case TypeKind_int:
        switch (from_type->kind) {
        case TypeKind_char:
        case TypeKind_int:
            // int -> char or short
            *expression = Sema_implicit_cast(
                s,
                destination_type,
                ValueCategory_rvalue,
                ImplicitCastOp_integral_cast,
                *expression);
            break;

        default:
            UNREACHABLE();
        }
        break;

    case TypeKind_pointer:
        switch (from_type->kind) {
        default:
            UNREACHABLE();
        }
        break;

    default:
        UNREACHABLE();
    }

    // TODO: assignment conversion
    Sema_decay_conversion(s, expression);
}

// Types
Type *Sema_act_on_struct_type_reference(Sema *s, const Token *identifier) {
    assert(s);
    assert(identifier);

    Symbol *symbol = Sema_find_struct_symbol(s, identifier->text);

    if (symbol == NULL) {
        Type *type = StructType_new();
        type->struct_symbol = symbol = Symbol_new(identifier->text, type);

        // Register the struct symbol
        Sema_register_struct_symbol(s, symbol);
    }

    return symbol->type;
}

Type *
Sema_act_on_struct_type_start_of_member_list(Sema *s, const Token *identifier) {
    assert(s);

    if (identifier == NULL) {
        UNIMPLEMENTED();
    }

    Type *type;
    Symbol *symbol = Sema_find_struct_symbol(s, identifier->text);

    if (symbol == NULL) {
        type = StructType_new();
        type->struct_symbol = symbol = Symbol_new(identifier->text, type);

        // Register the struct symbol
        Sema_register_struct_symbol(s, symbol);
    } else {
        type = symbol->type;

        if (StructType_is_defined(type)) {
            ERROR("multiple definition of struct %s\n", symbol->name);
        }
    }

    // Enter the struct member scope
    Sema_push_scope_stack_members(s);

    return type;
}

Type *Sema_act_on_struct_type_end_of_member_list(
    Sema *s, Type *struct_type, Vec(MemberDeclNode) * member_decls) {
    assert(s);
    assert(struct_type);
    assert(struct_type->kind == TypeKind_struct);
    assert(!StructType_is_defined(struct_type));
    assert(member_decls);

    Vec(Symbol) *member_symbols = Vec_new(Symbol)();

    for (size_t i = 0; i < Vec_len(MemberDeclNode)(member_decls); i++) {
        MemberDeclNode *decl = Vec_get(MemberDeclNode)(member_decls, i);
        size_t len = Vec_len(DeclaratorNode)(decl->declarators);

        for (size_t j = 0; j < len; j++) {
            DeclaratorNode *declarator =
                Vec_get(DeclaratorNode)(decl->declarators, j);
            Symbol *symbol = DeclaratorNode_symbol(declarator);

            Vec_push(Symbol)(member_symbols, symbol);
        }
    }

    if (Vec_len(Symbol)(member_symbols) == 0) {
        ERROR("struct cannot be empty\n");
    }

    // Leave the struct member scope
    Sema_pop_scope_stack_members(s);

    struct_type->member_decls = member_decls;
    struct_type->member_symbols = member_symbols;
    return struct_type;
}

MemberDeclNode *Sema_act_on_struct_type_member_decl(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(s);
    assert(decl_spec);
    assert(declarators);

    (void)s;

    // Type check
    for (size_t i = 0; i < Vec_len(DeclaratorNode)(declarators); i++) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);

        if (Type_is_incomplete_type(symbol->type)) {
            ERROR("member cannot have an incomplete\n");
        }
    }

    return MemberDeclNode_new(decl_spec, declarators);
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

    assert(symbol->type != NULL);

    // TODO: enumerator
    IdentifierExprNode *node =
        IdentifierExprNode_new(symbol->type, ValueCategory_lvalue, symbol);
    return IdentifierExprNode_base(node);
}

ExprNode *Sema_act_on_integer_expr(Sema *s, const Token *integer) {
    assert(s);
    assert(integer);
    assert(integer->kind == TokenKind_number);

    long long value = atoll(integer->text); // TODO: conversion

    IntegerExprNode *node =
        IntegerExprNode_new(s->int_type, ValueCategory_rvalue, value);
    return IntegerExprNode_base(node);
}

ExprNode *Sema_act_on_string_expr(Sema *s, const Token *string) {
    assert(s);
    assert(string);
    assert(string->kind == TokenKind_string);

    Type *type = ArrayType_new(s->char_type, string->string_len);

    StringExprNode *node = StringExprNode_new(
        type, ValueCategory_lvalue, string->string, string->string_len);
    return StringExprNode_base(node);
}

ExprNode *
Sema_act_on_subscript_expr(Sema *s, ExprNode *array, ExprNode *index) {
    assert(s);
    assert(array);
    assert(index);

    // Conversion
    Sema_integer_promotion(s, &array);
    Sema_integer_promotion(s, &index);

    // Type check
    if (array->result_type->kind != TypeKind_pointer ||
        index->result_type->kind != TypeKind_int) {
        ERROR("subscripted value is not a pointer\n");
    }

    Type *result_type = PointerType_pointee_type(array->result_type);

    if (Type_is_incomplete_type(result_type)) {
        ERROR("subscripted value is an incomplete type\n");
    }

    SubscriptExprNode *node =
        SubscriptExprNode_new(result_type, ValueCategory_lvalue, array, index);
    return SubscriptExprNode_base(node);
}

ExprNode *
Sema_act_on_call_expr(Sema *s, ExprNode *callee, Vec(ExprNode) * arguments) {
    assert(s);
    assert(callee);
    assert(arguments);

    // Conversion
    Sema_decay_conversion(s, &callee);

    // Type check
    if (!Type_is_function_pointer_type(callee->result_type)) {
        ERROR("cannot call a non-function type object\n");
    }

    Type *function_type = PointerType_pointee_type(callee->result_type);
    Type *return_type = FunctionType_return_type(function_type);
    const Vec(Type) *parameter_types =
        FunctionType_parameter_types(function_type);
    size_t num_parameters = Vec_len(Type)(parameter_types);
    bool var_arg = false; // TODO: var args

    size_t num_arguments = Vec_len(ExprNode)(arguments);

    if (num_arguments < num_parameters) {
        if (var_arg) {
            ERROR(
                "too few arguments to function call, expected at least %zu, "
                "have %zu\n",
                num_parameters,
                num_arguments);
        } else {
            ERROR(
                "too few arguments to function call, expected %zu, have %zu\n",
                num_parameters,
                num_arguments);
        }
    } else if (num_arguments > num_parameters && !var_arg) {
        ERROR(
            "too much arguments to function call, expected %zu, have %zu\n",
            num_parameters,
            num_arguments);
    }

    for (size_t i = 0; i < num_arguments; i++) {
        ExprNode *argument = Vec_get(ExprNode)(arguments, i);

        if (i < num_parameters) {
            // Assignment conversion
            Type *parameter_type = Vec_get(Type)(parameter_types, i);

            Sema_assignment_conversion(s, parameter_type, &argument);
        } else {
            TODO("default argument promotion");
        }

        Vec_set(ExprNode)(arguments, i, argument);
    }

    CallExprNode *node =
        CallExprNode_new(return_type, ValueCategory_rvalue, callee, arguments);
    return CallExprNode_base(node);
}

ExprNode *
Sema_act_on_dot_expr(Sema *s, ExprNode *parent, const Token *identifier) {
    assert(s);
    assert(parent);
    assert(identifier);

    (void)s;

    Type *struct_type = parent->result_type;

    // Type check
    if (struct_type->kind != TypeKind_struct) {
        ERROR("cannot access member of non-struct type\n");
    }

    if (!StructType_is_defined(struct_type)) {
        ERROR("cannot access member of incomplete struct type\n");
    }

    // Find the member symbol
    Symbol *symbol = StructType_find_member(struct_type, identifier->text);

    if (symbol == NULL) {
        ERROR("cannot find member named %s\n", identifier->text);
    }

    DotExprNode *node =
        DotExprNode_new(symbol->type, parent->value_category, parent, symbol);
    return DotExprNode_base(node);
}

ExprNode *
Sema_act_on_arrow_expr(Sema *s, ExprNode *parent, const Token *identifier) {
    assert(s);
    assert(parent);
    assert(identifier);

    (void)s;

    // Type conversion
    Sema_decay_conversion(s, &parent);

    // Type check
    Type *pointer_type = parent->result_type;

    if (pointer_type->kind != TypeKind_pointer) {
        ERROR("cannot access member of non-struct pointer type\n");
    }

    Type *struct_type = PointerType_pointee_type(pointer_type);

    if (struct_type->kind != TypeKind_struct) {
        ERROR("cannot access member of non-struct type\n");
    }

    if (!StructType_is_defined(struct_type)) {
        ERROR("cannot access member of incomplete struct type\n");
    }

    // Find the member symbol
    Symbol *symbol = StructType_find_member(struct_type, identifier->text);

    if (symbol == NULL) {
        ERROR("cannot find member named %s\n", identifier->text);
    }

    ArrowExprNode *node =
        ArrowExprNode_new(symbol->type, ValueCategory_lvalue, parent, symbol);
    return ArrowExprNode_base(node);
}

ExprNode *
Sema_act_on_unary_expr(Sema *s, const Token *operator, ExprNode *operand) {
    assert(s);
    assert(operator);
    assert(operand);

    Type *type;
    ValueCategory value_category;
    UnaryOp op;
    switch (operator->kind) {
    case '+':
    case '-':
        TODO("unary + -");
        break;

    case '!':
        TODO("unary !");
        break;

    case '&':
        if (operand->value_category != ValueCategory_lvalue) {
            ERROR("cannot take the address of an rvalue\n");
        }

        type = PointerType_new(operand->result_type);
        value_category = ValueCategory_rvalue;
        op = UnaryOp_address_of;
        break;

    case '*':
        // Conversion
        Sema_decay_conversion(s, &operand);

        if (operand->result_type->kind != TypeKind_pointer) {
            ERROR("indirection requires pointer operand\n");
        }

        type = PointerType_pointee_type(operand->result_type);
        value_category = ValueCategory_lvalue;
        op = UnaryOp_indirection;
        break;

    case TokenKind_kw_sizeof:
        TODO("unary sizeof");
        break;

    default:
        ERROR("unknown unary operator %s\n", operator->text);
    }

    UnaryExprNode *node = UnaryExprNode_new(type, value_category, op, operand);
    return UnaryExprNode_base(node);
}

ExprNode *Sema_act_on_binary_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs) {
    assert(s);
    assert(lhs);
    assert(operator);
    assert(rhs);

    Type *type;
    BinaryOp op;
    switch (operator->kind) {
    case '+':
        // Promotion
        Sema_usual_arithmetic_conversion(s, &lhs, &rhs);

        // Type check
        if (lhs->result_type->kind == TypeKind_int &&
            rhs->result_type->kind == TypeKind_int) {
        } else {
            ERROR("invalid operands to binary +\n");
        }

        type = s->int_type;
        op = BinaryOp_add;
        break;

    case '-':
        // Promotion
        Sema_usual_arithmetic_conversion(s, &lhs, &rhs);

        // Type check
        if (lhs->result_type->kind == TypeKind_int &&
            rhs->result_type->kind == TypeKind_int) {
        } else {
            ERROR("invalid operands to binary -\n");
        }

        type = s->int_type;
        op = BinaryOp_sub;
        break;

    default:
        ERROR("unknown binary operator %s\n", operator->text);
    }

    BinaryExprNode *node =
        BinaryExprNode_new(type, ValueCategory_rvalue, op, lhs, rhs);
    return BinaryExprNode_base(node);
}

ExprNode *Sema_act_on_assign_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs) {
    assert(s);
    assert(lhs);
    assert(operator);
    assert(rhs);

    if (lhs->value_category != ValueCategory_lvalue) {
        ERROR("expression is not assignable\n");
    }

    switch (operator->kind) {
    case '=':
        // Conversion
        Sema_assignment_conversion(s, lhs->result_type, &rhs);
        break;

    default:
        ERROR("unknown assignment operator %s\n", operator->text);
    }

    // Assignments is rvalue in C
    ValueCategory value_category = ValueCategory_rvalue;

    AssignExprNode *node =
        AssignExprNode_new(lhs->result_type, value_category, lhs, rhs);
    return AssignExprNode_base(node);
}

// Statements
StmtNode *Sema_act_on_if_stmt(
    Sema *s, ExprNode *condition, StmtNode *if_true, StmtNode *if_false) {
    assert(s);
    assert(condition);
    assert(if_true);

    // Conversion
    Sema_decay_conversion(s, &condition);

    // TODO: Type check

    IfStmtNode *node = IfStmtNode_new(condition, if_true, if_false);
    return IfStmtNode_base(node);
}

StmtNode *Sema_act_on_return_stmt(Sema *s, ExprNode *return_value) {
    assert(s);

    // TODO: Type check
    if (return_value != NULL) {
        // Conversion
        Sema_assignment_conversion(s, s->return_type, &return_value);
    }

    ReturnStmtNode *node = ReturnStmtNode_new(return_value);
    return ReturnStmtNode_base(node);
}

StmtNode *Sema_act_on_decl_stmt(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(s);
    assert(decl_spec);
    assert(declarators);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(declarators); i++) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);

        Vec_push(Symbol)(s->local_variables, symbol);
    }

    DeclStmtNode *node = DeclStmtNode_new(decl_spec, declarators);
    return DeclStmtNode_base(node);
}

StmtNode *Sema_act_on_expr_stmt(Sema *s, ExprNode *expression) {
    assert(s);
    assert(expression);

    (void)s;

    ExprStmtNode *node = ExprStmtNode_new(expression);
    return ExprStmtNode_base(node);
}

// Declarators
DeclaratorNode *
Sema_act_on_direct_declarator(Sema *s, const Token *identifier) {
    assert(s);
    assert(identifier);

    (void)s;

    DirectDeclaratorNode *node = DirectDeclaratorNode_new(identifier->text);
    return DirectDeclaratorNode_base(node);
}

DeclaratorNode *Sema_act_on_array_declarator(
    Sema *s, DeclaratorNode *declarator, ExprNode *array_size) {
    assert(s);
    assert(declarator);
    assert(array_size);

    (void)s;

    ArrayDeclaratorNode *node = ArrayDeclaratorNode_new(declarator, array_size);
    return ArrayDeclaratorNode_base(node);
}

void Sema_act_on_function_declarator_start_of_parameter_list(Sema *s) {
    assert(s);

    // Enter the parameter scope
    Sema_push_scope_stack(s);
}

DeclaratorNode *Sema_act_on_function_declarator_end_of_parameter_list(
    Sema *s, DeclaratorNode *declarator, Vec(DeclaratorNode) * parameters) {
    assert(s);
    assert(declarator);
    assert(parameters);

    // Leave the parameter scope
    Sema_pop_scope_stack(s);

    FunctionDeclaratorNode *node =
        FunctionDeclaratorNode_new(declarator, parameters);
    return FunctionDeclaratorNode_base(node);
}

DeclaratorNode *
Sema_act_on_pointer_declarator(Sema *s, DeclaratorNode *declarator) {
    assert(s);
    assert(declarator);

    (void)s;

    PointerDeclaratorNode *node = PointerDeclaratorNode_new(declarator);
    return PointerDeclaratorNode_base(node);
}

static void
Sema_complete_declarator(Sema *s, DeclaratorNode *declarator, Type *base_type);

static void Sema_complete_declarator_Direct(
    Sema *s, DirectDeclaratorNode *declarator, Type *base_type) {
    assert(s);
    assert(declarator);
    assert(base_type);

    if (base_type->kind == TypeKind_function) {
        // Function
        declarator->symbol = Sema_find_symbol(s, declarator->name);

        if (declarator->symbol == NULL) {
            declarator->symbol = Symbol_new(declarator->name, base_type);

            Sema_register_symbol(s, declarator->symbol);
        } else if (!Type_equals(declarator->symbol->type, base_type)) {
            ERROR("conflict function type of %s\n", declarator->symbol->name);
        }
    } else {
        // Variable
        declarator->symbol = Symbol_new(declarator->name, base_type);

        Sema_register_symbol(s, declarator->symbol);
    }
}

static void Sema_complete_declarator_Array(
    Sema *s, ArrayDeclaratorNode *declarator, Type *base_type) {
    assert(s);
    assert(declarator);
    assert(base_type);

    // Type check
    if (Type_is_incomplete_type(base_type)) {
        ERROR("array has incomplete element type\n");
    }

    // TODO: unsized array
    if (declarator->array_size == NULL) {
        TODO("array without size");
    }

    // TODO: constant value
    long long length = IntegerExprNode_ccast(declarator->array_size)->value;

    if (length <= 0) {
        ERROR("array must have a positive length\n");
    }

    base_type = ArrayType_new(base_type, length);

    Sema_complete_declarator(s, declarator->declarator, base_type);
}

static void Sema_complete_declarator_Function(
    Sema *s, FunctionDeclaratorNode *declarator, Type *base_type) {
    assert(s);
    assert(declarator);
    assert(base_type);

    Vec(Type) *parameter_types = Vec_new(Type)();
    for (size_t i = 0; i < Vec_len(DeclaratorNode)(declarator->parameters);
         i++) {
        DeclaratorNode *parameter =
            Vec_get(DeclaratorNode)(declarator->parameters, i);
        Symbol *symbol = DeclaratorNode_symbol(parameter);

        Vec_push(Type)(parameter_types, symbol->type);
    }

    // Type check
    if (base_type->kind == TypeKind_function) {
        ERROR("function cannot return a function\n");
    } else if (base_type->kind == TypeKind_array) {
        ERROR("function cannot return an array\n");
    }

    base_type = FunctionType_new(base_type, parameter_types);

    Sema_complete_declarator(s, declarator->declarator, base_type);
}

static void Sema_complete_declarator_Pointer(
    Sema *s, PointerDeclaratorNode *declarator, Type *base_type) {
    assert(s);
    assert(declarator);
    assert(base_type);

    Sema_complete_declarator(
        s, declarator->declarator, PointerType_new(base_type));
}

static void Sema_complete_declarator_Init(
    Sema *s, InitDeclaratorNode *declarator, Type *base_type) {
    (void)s;
    (void)declarator;
    (void)base_type;
    UNREACHABLE();
}

static void
Sema_complete_declarator(Sema *s, DeclaratorNode *declarator, Type *base_type) {
    assert(s);
    assert(declarator);

    switch (declarator->kind) {
#define DECLARATOR_NODE(name)                                                  \
    case NodeKind_##name##Declarator:                                          \
        Sema_complete_declarator_##name(                                       \
            s, name##DeclaratorNode_cast(declarator), base_type);              \
        break;
#include "Ast.def"

    default:
        UNREACHABLE();
    }
}

DeclaratorNode *Sema_act_on_declarator_completed(
    Sema *s, DeclSpecNode *decl_spec, DeclaratorNode *declarator) {
    assert(s);
    assert(decl_spec);
    assert(declarator);

    Sema_complete_declarator(s, declarator, decl_spec->base_type);
    return declarator;
}

DeclaratorNode *Sema_act_on_init_declarator(
    Sema *s, DeclaratorNode *declarator, ExprNode *initializer) {
    assert(s);
    assert(declarator);
    assert(initializer);

    Symbol *symbol = DeclaratorNode_symbol(declarator);

    // Conversion
    Sema_assignment_conversion(s, symbol->type, &initializer);

    InitDeclaratorNode *node = InitDeclaratorNode_new(declarator, initializer);
    return InitDeclaratorNode_base(node);
}

// Declarations
DeclSpecNode *Sema_act_on_decl_spec(Sema *s, Type *base_type) {
    assert(s);
    assert(base_type);

    (void)s;
    return DeclSpecNode_new(base_type);
}

DeclaratorNode *
Sema_act_on_parameter_decl(Sema *s, DeclaratorNode *declarator) {
    assert(s);
    assert(declarator);

    // TODO: Type check
    // TODO: Type conversion
    (void)s;

    return declarator;
}

DeclNode *Sema_act_on_global_decl(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(s);
    assert(decl_spec);
    assert(declarators);

    (void)s;

    GlobalDeclNode *node = GlobalDeclNode_new(decl_spec, declarators);
    return GlobalDeclNode_base(node);
}

void Sema_act_on_function_decl_start_of_body(
    Sema *s, DeclSpecNode *decl_spec, DeclaratorNode *declarator) {
    assert(s);
    assert(decl_spec);
    assert(declarator);

    (void)decl_spec;

    // Enter the function scope
    Sema_push_scope_stack(s);

    // Register the parameters to the scope
    Symbol *symbol = DeclaratorNode_symbol(declarator);
    Vec(DeclaratorNode) *parameters = DeclaratorNode_parameters(declarator);

    if (symbol->type->kind != TypeKind_function || parameters == NULL) {
        ERROR("declarator is not a function\n");
    }

    if (symbol->has_body) {
        ERROR("multiple definition of function %s\n", symbol->name);
    }

    symbol->has_body = true;

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(parameters); i++) {
        DeclaratorNode *parameter = Vec_get(DeclaratorNode)(parameters, i);
        Symbol *parameter_symbol = DeclaratorNode_symbol(parameter);

        Sema_register_symbol(s, parameter_symbol);
    }

    s->local_variables = Vec_new(Symbol)();
    s->return_type = FunctionType_return_type(symbol->type);
}

DeclNode *Sema_act_on_function_decl_end_of_body(
    Sema *s,
    DeclSpecNode *decl_spec,
    DeclaratorNode *declarator,
    StmtNode *body) {
    assert(s);
    assert(decl_spec);
    assert(declarator);
    assert(body);

    // Leave the function scope
    Sema_pop_scope_stack(s);

    Vec(Symbol) *local_variables = s->local_variables;
    s->local_variables = NULL;

    FunctionDeclNode *node =
        FunctionDeclNode_new(decl_spec, declarator, body, local_variables);
    return FunctionDeclNode_base(node);
}
