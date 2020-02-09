#include "mocc.h"

struct Parser {
    Sema *sema;
    const Vec(Token) * tokens;
    size_t cursor;
};

Parser *Parser_new(const Vec(Token) * tokens) {
    assert(tokens);

    Parser *p = malloc(sizeof(Parser));
    p->sema = Sema_new();
    p->tokens = tokens;
    p->cursor = 0;

    return p;
}

static const Token *Parser_current(const Parser *p) {
    assert(p);

    return Vec_get(Token)(p->tokens, p->cursor);
}

static const Token *Parser_peek(const Parser *p) {
    assert(p);

    return Vec_get(Token)(p->tokens, p->cursor + 1);
}

static const Token *Parser_consume(Parser *p) {
    assert(p);
    assert(Parser_current(p)->kind != '\0');

    return Vec_get(Token)(p->tokens, p->cursor++);
}

static const Token *Parser_expect(Parser *p, TokenKind kind) {
    assert(p);

    const Token *t = Parser_current(p);
    if (t->kind != kind) {
        ERROR("unexpected token %s, expected %d\n", t->text, kind);
    }

    return Parser_consume(p);
}

static bool Parser_is_decl_spec(Parser *p, const Token *t) {
    assert(p);
    assert(t);

    switch (t->kind) {
    case TokenKind_kw_void:
    case TokenKind_kw_char:
    case TokenKind_kw_int:
    case TokenKind_kw_struct:
    case TokenKind_kw_enum:
        return true;

    case TokenKind_identifier:
        return Sema_is_typedef_name(p->sema, t);

    default:
        return false;
    }
}

static DeclSpecNode *
Parser_parse_type_spec(Parser *p, StorageClass storage_class);
static ExprNode *Parser_parse_assign_expr(Parser *p);
static ExprNode *Parser_parse_comma_expr(Parser *p);
static StmtNode *Parser_parse_stmt(Parser *p);
static DeclaratorNode *
Parser_parse_declarator(Parser *p, DeclSpecNode *decl_spec);
static DeclaratorNode *
Parser_parse_init_declarator(Parser *p, DeclSpecNode *decl_spec);

// struct_type:
//  'struct' identifier [member_list]
//  'struct' member_list
//
// member_list:
//  type_spec [declarator (',' declarator)*] ';'
static Type *Parser_parse_struct_type(Parser *p) {
    assert(p);

    // 'struct'
    Parser_expect(p, TokenKind_kw_struct);

    // TODO: Struct definition without name

    // identifier
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    // [member_list]
    if (Parser_current(p)->kind != '{') {
        return Sema_act_on_struct_type_reference(p->sema, identifier);
    }

    Type *type =
        Sema_act_on_struct_type_start_of_member_list(p->sema, identifier);

    // member_list
    Vec(MemberDeclNode) *member_decls = Vec_new(MemberDeclNode)();

    // '{'
    Parser_expect(p, '{');

    while (Parser_current(p)->kind != '}') {
        // member_decl
        // type_spec
        DeclSpecNode *decl_spec = Parser_parse_type_spec(p, StorageClass_none);

        // [declarator (',' declarator)*]
        Vec(DeclaratorNode) *declarators = Vec_new(DeclaratorNode)();

        if (Parser_current(p)->kind != ';') {
            // declarator
            Vec_push(DeclaratorNode)(
                declarators, Parser_parse_declarator(p, decl_spec));

            // (',' declarator)*
            while (Parser_current(p)->kind == ',') {
                // ','
                Parser_expect(p, ',');

                // declarator
                Vec_push(DeclaratorNode)(
                    declarators, Parser_parse_declarator(p, decl_spec));
            }
        }

        // ';'
        Parser_expect(p, ';');

        Vec_push(MemberDeclNode)(
            member_decls,
            Sema_act_on_struct_type_member_decl(
                p->sema, decl_spec, declarators));
    }

    // '}'
    Parser_expect(p, '}');

    return Sema_act_on_struct_type_end_of_member_list(
        p->sema, type, member_decls);
}

// enumerator:
//  identifier ['=' assign_expr]
EnumeratorDeclNode *Parser_parse_enumerator(Parser *p) {
    assert(p);

    // identifier
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    // ['=' assign_expr]
    ExprNode *value = NULL;

    if (Parser_current(p)->kind == '=') {
        // '='
        Parser_expect(p, '=');

        // assign_expr
        value = Parser_parse_assign_expr(p);
    }

    return Sema_act_on_enumerator(p->sema, identifier, value);
}

// enum_type:
//  'enum' identifier [enumerator_list]
//  'enum' enumerator_list
//
// enumerator_list:
//  '{' enumerator (',' enumerator)* [','] '}'
static Type *Parser_parse_enum_type(Parser *p) {
    assert(p);

    // 'enum'
    Parser_expect(p, TokenKind_kw_enum);

    // TODO: Enum definition without name
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    // [enumerator_list]
    if (Parser_current(p)->kind != '{') {
        return Sema_act_on_enum_type_reference(p->sema, identifier);
    }

    Sema_act_on_enum_type_start_of_list(p->sema, identifier);

    // enumerator_list
    Vec(EnumeratorDeclNode) *enumerators = Vec_new(EnumeratorDeclNode)();

    // '{'
    Parser_expect(p, '{');

    // enumerator
    Vec_push(EnumeratorDeclNode)(enumerators, Parser_parse_enumerator(p));

    // (',' enumerator)*
    while (Parser_current(p)->kind == ',' && Parser_peek(p)->kind != '}') {
        // ','
        Parser_expect(p, ',');

        // enumerator
        Vec_push(EnumeratorDeclNode)(enumerators, Parser_parse_enumerator(p));
    }

    // [',']
    if (Parser_current(p)->kind == ',') {
        Parser_expect(p, ',');
    }

    // '}'
    Parser_expect(p, '}');

    return Sema_act_on_enum_type_end_of_list(p->sema, identifier, enumerators);
}

// type_spec:
//  [type_specifier] type
//
// type_specifier:
//  'const'
static DeclSpecNode *
Parser_parse_type_spec(Parser *p, StorageClass storage_class) {
    assert(p);

    // [type_specifier]
    switch (Parser_current(p)->kind) {
    case TokenKind_kw_const:
        // 'const'
        Parser_expect(p, TokenKind_kw_const); // TODO: const
        break;

    default:
        break;
    }

    // type
    Type *base_type;

    switch (Parser_current(p)->kind) {
    case TokenKind_kw_void:
        // 'void'
        Parser_expect(p, TokenKind_kw_void);

        base_type = VoidType_new();
        break;

    case TokenKind_kw_char:
        // 'char'
        Parser_expect(p, TokenKind_kw_char);

        base_type = CharType_new();
        break;

    case TokenKind_kw_int:
        // 'int'
        Parser_expect(p, TokenKind_kw_int);

        base_type = IntType_new();
        break;

    case TokenKind_kw_struct:
        // 'struct'
        base_type = Parser_parse_struct_type(p);
        break;

    case TokenKind_kw_enum:
        // 'enum'
        base_type = Parser_parse_enum_type(p);
        break;

    case TokenKind_identifier: {
        // identifier
        const Token *identifier = Parser_expect(p, TokenKind_identifier);

        base_type = Sema_act_on_typedef_name(p->sema, identifier);
        break;
    }

    default:
        ERROR("expected type, but got %s\n", Parser_current(p)->text);
        break;
    }

    return Sema_act_on_decl_spec(p->sema, storage_class, base_type);
}

// decl_spec:
//  [storage_class] type_spec
//
// storage_class:
//  'static'
//  'typedef'
static DeclSpecNode *Parser_parse_decl_spec(Parser *p) {
    assert(p);

    // [storage_class]
    StorageClass storage_class;

    switch (Parser_current(p)->kind) {
    case TokenKind_kw_static:
        // 'static'
        Parser_expect(p, TokenKind_kw_static);

        storage_class = StorageClass_static;
        break;

    case TokenKind_kw_typedef:
        // 'typedef'
        Parser_expect(p, TokenKind_kw_typedef);

        storage_class = StorageClass_typedef;
        break;

    default:
        storage_class = StorageClass_none;
        break;
    }

    return Parser_parse_type_spec(p, storage_class);
}

// identifier_expr:
//  identifier
static ExprNode *Parser_parse_identifier_expr(Parser *p) {
    assert(p);

    // identifier
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    return Sema_act_on_identifier_expr(p->sema, identifier);
}

// number_expr:
//  number
static ExprNode *Parser_parse_number_expr(Parser *p) {
    assert(p);

    // number
    const Token *num = Parser_expect(p, TokenKind_number);

    return Sema_act_on_integer_expr(p->sema, num);
}

// string_expr:
//  string
static ExprNode *Parser_parse_string_expr(Parser *p) {
    assert(p);

    // string
    const Token *string = Parser_expect(p, TokenKind_string);

    return Sema_act_on_string_expr(p->sema, string);
}

// paren_expr:
//  '(' comma_expr ')'
static ExprNode *Parser_parse_paren_expr(Parser *p) {
    assert(p);

    // '('
    Parser_expect(p, '(');

    // comma_expr
    ExprNode *expression = Parser_parse_comma_expr(p);

    // ')'
    Parser_expect(p, ')');

    return expression;
}

// primary_expr:
//  identifier_expr
//  number_expr
//  string_expr
//  paren_expr
static ExprNode *Parser_parse_primary_expr(Parser *p) {
    assert(p);

    const Token *t = Parser_current(p);
    switch (t->kind) {
    case TokenKind_identifier:
        // identifier_expr
        return Parser_parse_identifier_expr(p);

    case TokenKind_number:
        // number_expr
        return Parser_parse_number_expr(p);

    case TokenKind_string:
        // string_expr
        return Parser_parse_string_expr(p);

    case '(':
        // paren_expr
        return Parser_parse_paren_expr(p);

    default:
        ERROR("unexpected token %s, expected expression\n", t->text);
    }
}

// subscript_expr:
//  postfix_expr '[' comma_expr ']'
static ExprNode *Parser_parse_subscript_expr(Parser *p, ExprNode *array) {
    assert(p);
    assert(array);

    // '['
    Parser_expect(p, '[');

    // comma_expr
    ExprNode *index = Parser_parse_comma_expr(p);

    // ']'
    Parser_expect(p, ']');

    return Sema_act_on_subscript_expr(p->sema, array, index);
}

// call_expr:
//  postfix_expr '(' [assign_expr (',' assign_expr)*] ')'
static ExprNode *Parser_parse_call_expr(Parser *p, ExprNode *callee) {
    assert(p);
    assert(callee);

    // '('
    Parser_expect(p, '(');

    // [assign_expr (',' assign_expr)*]
    Vec(ExprNode) *arguments = Vec_new(ExprNode)();

    if (Parser_current(p)->kind != ')') {
        // assign_expr
        Vec_push(ExprNode)(arguments, Parser_parse_assign_expr(p));

        // (',' assign_expr)*
        while (Parser_current(p)->kind == ',') {
            // ','
            Parser_expect(p, ',');

            // assign_expr
            Vec_push(ExprNode)(arguments, Parser_parse_assign_expr(p));
        }
    }

    // ')'
    Parser_expect(p, ')');

    return Sema_act_on_call_expr(p->sema, callee, arguments);
}

// dot_expr:
//  postfix_expr '.' identifier
static ExprNode *Parser_parse_dot_expr(Parser *p, ExprNode *parent) {
    assert(p);
    assert(parent);

    // '.'
    Parser_expect(p, '.');

    // identifier
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    return Sema_act_on_dot_expr(p->sema, parent, identifier);
}

// arrow_expr:
//  postfix_expr '->' identifier
static ExprNode *Parser_parse_arrow_expr(Parser *p, ExprNode *parent) {
    assert(p);
    assert(parent);

    // '->'
    Parser_expect(p, TokenKind_arrow);

    // identifier
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    return Sema_act_on_arrow_expr(p->sema, parent, identifier);
}

// postfix_expr:
//  subscript_expr
//  call_expr
//  dot_expr
//  primary_expr
static ExprNode *Parser_parse_postfix_expr(Parser *p) {
    assert(p);

    // primary_expr
    ExprNode *node = Parser_parse_primary_expr(p);

    while (true) {
        switch (Parser_current(p)->kind) {
        case '[':
            node = Parser_parse_subscript_expr(p, node);
            break;

        case '(':
            node = Parser_parse_call_expr(p, node);
            break;

        case '.':
            node = Parser_parse_dot_expr(p, node);
            break;

        case TokenKind_arrow:
            node = Parser_parse_arrow_expr(p, node);
            break;

        default:
            return node;
        }
    }
}

static ExprNode *Parser_parse_unary_expr(Parser *p);

// sizeof_expr:
//  'sizeof' '(' type_spec ')'
//  'sizeof' unary_expr
static ExprNode *Parser_parse_sizeof_expr(Parser *p) {
    assert(p);

    // 'sizeof'
    Parser_expect(p, TokenKind_kw_sizeof);

    if (Parser_current(p)->kind == '(' &&
        Parser_is_decl_spec(p, Parser_peek(p))) {
        // '('
        Parser_expect(p, '(');

        // type
        DeclSpecNode *decl_spec = Parser_parse_type_spec(p, StorageClass_none);

        // ')'
        Parser_expect(p, ')');

        return Sema_act_on_sizeof_expr_type(p->sema, decl_spec->base_type);
    } else {
        // unary_expr
        ExprNode *expression = Parser_parse_unary_expr(p);

        return Sema_act_on_sizeof_expr_expr(p->sema, expression);
    }
}

// unary_expr:
//  '+' unary_expr
//  '-' unary_expr
//  '!' unary_expr
//  '&' unary_expr
//  '*' unary_expr
//  'sizeof' unary_expr
//  postfix_expr
static ExprNode *Parser_parse_unary_expr(Parser *p) {
    assert(p);

    switch (Parser_current(p)->kind) {
    case '+':
    case '-':
    case '!':
    case '&':
    case '*': {
        // unary_op
        const Token *operator= Parser_consume(p);

        // unary_expr
        ExprNode *operand = Parser_parse_unary_expr(p);

        return Sema_act_on_unary_expr(p->sema, operator, operand);
    }

    case TokenKind_kw_sizeof:
        // sizeof_expr
        return Parser_parse_sizeof_expr(p);

    default:
        // postfix_expr
        return Parser_parse_postfix_expr(p);
    }
}

// multiplicative_expr:
//  multiplicative_expr '+' unary_expr
//  multiplicative_expr '-' unary_expr
//  unary_expr
static ExprNode *Parser_parse_multiplicative_expr(Parser *p) {
    assert(p);

    // unary_expr
    ExprNode *lhs = Parser_parse_unary_expr(p);

    while (Parser_current(p)->kind == '*' || Parser_current(p)->kind == '/' ||
           Parser_current(p)->kind == '%') {
        // '*' | '/' | '%'
        const Token *operator= Parser_consume(p);

        // unary_expr
        ExprNode *rhs = Parser_parse_unary_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// additive_expr:
//  additive_expr '+' multiplicative_expr
//  additive_expr '-' multiplicative_expr
//  multiplicative_expr
static ExprNode *Parser_parse_additive_expr(Parser *p) {
    assert(p);

    // multiplicative_expr
    ExprNode *lhs = Parser_parse_multiplicative_expr(p);

    while (Parser_current(p)->kind == '+' || Parser_current(p)->kind == '-') {
        // '+' | '-'
        const Token *operator= Parser_consume(p);

        // multiplicative_expr
        ExprNode *rhs = Parser_parse_multiplicative_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// shift_expr:
//  shift_expr '<<' additive_expr
//  shift_expr '>>' additive_expr
//  additive_expr
static ExprNode *Parser_parse_shift_expr(Parser *p) {
    assert(p);

    // TODO: shift_expr
    return Parser_parse_additive_expr(p);
}

// relational_expr:
//  relational_expr '<' shift_expr
//  relational_expr '>' shift_expr
//  relational_expr '<=' shift_expr
//  relational_expr '>=' shift_expr
//  shift_expr
static ExprNode *Parser_parse_relational_expr(Parser *p) {
    assert(p);

    // shift_expr
    ExprNode *lhs = Parser_parse_shift_expr(p);

    while (Parser_current(p)->kind == '<' ||
           Parser_current(p)->kind == TokenKind_lesser_equal ||
           Parser_current(p)->kind == '>' ||
           Parser_current(p)->kind == TokenKind_greater_equal) {
        // '<' | '<=' | '>' | '>='
        const Token *operator= Parser_consume(p);

        // shift_expr
        ExprNode *rhs = Parser_parse_shift_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// equality_expr:
//  equality_expr '==' relational_expr
//  equality_expr '!=' relational_expr
//  relational_expr
static ExprNode *Parser_parse_equality_expr(Parser *p) {
    assert(p);

    // relational_expr
    ExprNode *lhs = Parser_parse_relational_expr(p);

    while (Parser_current(p)->kind == TokenKind_equal ||
           Parser_current(p)->kind == TokenKind_not_equal) {
        // '==' | '!='
        const Token *operator= Parser_consume(p);

        // relational_expr
        ExprNode *rhs = Parser_parse_relational_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// or_expr:
//  or_expr '|' xor_expr
//  xor_expr
static ExprNode *Parser_parse_or_expr(Parser *p) {
    assert(p);

    // TODO: or_expr
    return Parser_parse_equality_expr(p);
}

// logical_and_expr:
//  logical_and_expr '||' or_expr
//  or_expr
static ExprNode *Parser_parse_logical_and_expr(Parser *p) {
    assert(p);

    // or_expr
    ExprNode *lhs = Parser_parse_or_expr(p);

    while (Parser_current(p)->kind == TokenKind_and_and) {
        // '|'
        const Token *operator= Parser_consume(p);

        // or_expr
        ExprNode *rhs = Parser_parse_or_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// logical_or_expr:
//  logical_or_expr '||' logical_and_expr
//  logical_and_expr
static ExprNode *Parser_parse_logical_or_expr(Parser *p) {
    assert(p);

    // logical_and_expr
    ExprNode *lhs = Parser_parse_logical_and_expr(p);

    while (Parser_current(p)->kind == TokenKind_or_or) {
        // '||'
        const Token *operator= Parser_consume(p);

        // logical_and_expr
        ExprNode *rhs = Parser_parse_logical_and_expr(p);

        lhs = Sema_act_on_binary_expr(p->sema, lhs, operator, rhs);
    }

    return lhs;
}

// conditional_expr:
//  logical_or_expr '?' comma_expression ':' conditional_expr
//  logical_or_expr
static ExprNode *Parser_parse_conditional_expr(Parser *p) {
    assert(p);

    // logical_or_expr
    ExprNode *condition = Parser_parse_logical_or_expr(p);

    // ['?' comma_expression ':' conditional_expr]
    if (Parser_current(p)->kind != '?') {
        return condition;
    }

    // '?'
    Parser_expect(p, '?');

    // comma_expression
    ExprNode *if_true = Parser_parse_comma_expr(p);

    // ':'
    Parser_expect(p, ':');

    // conditional_expr
    ExprNode *if_false = Parser_parse_conditional_expr(p);

    (void)if_true;
    (void)if_false;
    UNIMPLEMENTED();
}

// assign_expr:
//  conditional_expr '=' assign_expr
//  conditional_expr
static ExprNode *Parser_parse_assign_expr(Parser *p) {
    assert(p);

    // conditional_expr
    ExprNode *lhs = Parser_parse_conditional_expr(p);

    if (Parser_current(p)->kind != '=') {
        return lhs;
    }

    // '='
    const Token *operator= Parser_expect(p, '=');

    // assign_expr
    ExprNode *rhs = Parser_parse_assign_expr(p);

    return Sema_act_on_assign_expr(p->sema, lhs, operator, rhs);
}

// comma_expr:
//  comma_expr ',' assign_expr
//  assign_expr
static ExprNode *Parser_parse_comma_expr(Parser *p) {
    assert(p);

    // TODO: comma_expr
    return Parser_parse_assign_expr(p);
}

// compound_stmt:
//  '{' stmt* '}'
static StmtNode *Parser_parse_compound_stmt(Parser *p) {
    assert(p);

    // '{'
    Parser_expect(p, '{');

    // stmt*
    Vec(StmtNode) *statements = Vec_new(StmtNode)();

    while (Parser_current(p)->kind != '}') {
        StmtNode *stmt = Parser_parse_stmt(p);

        Vec_push(StmtNode)(statements, stmt);
    }

    // '}'
    Parser_expect(p, '}');

    return CompoundStmtNode_base(CompoundStmtNode_new(statements));
}

// if_stmt:
//  'if' '(' comma_expr ')' stmt 'else' stmt
//  'if' '(' comma_expr ')' stmt
static StmtNode *Parser_parse_if_stmt(Parser *p) {
    assert(p);

    // 'if'
    Parser_expect(p, TokenKind_kw_if);

    // '('
    Parser_expect(p, '(');

    // comma_expr
    ExprNode *condition = Parser_parse_comma_expr(p);

    // ')'
    Parser_expect(p, ')');

    // stmt
    StmtNode *if_true = Parser_parse_stmt(p);

    // ['else' stmt]
    StmtNode *if_false = NULL;

    if (Parser_current(p)->kind == TokenKind_kw_else) {
        // 'else'
        Parser_expect(p, TokenKind_kw_else);

        // stmt
        if_false = Parser_parse_stmt(p);
    }

    return Sema_act_on_if_stmt(p->sema, condition, if_true, if_false);
}

// while_stmt:
//  'while' '(' comma_expr ')' stmt
static StmtNode *Parser_parse_while_stmt(Parser *p) {
    assert(p);

    // 'while'
    Parser_expect(p, TokenKind_kw_while);

    // '('
    Parser_expect(p, '(');

    // comma_expr
    ExprNode *condition = Parser_parse_comma_expr(p);

    // ')'
    Parser_expect(p, ')');

    // stmt
    StmtNode *body = Parser_parse_stmt(p);

    return Sema_act_on_while_stmt(p->sema, condition, body);
}

// for_stmt:
//  'for' '(' [decl_or_expr] ';' [comma_expr] ';' [comma_expr] ')' stmt
//
// decl_or_expr:
//  type_spec [init_declarator (',' init_declarator)*]
//  comma_expr
static StmtNode *Parser_parse_for_stmt(Parser *p) {
    assert(p);

    Sema_act_on_for_stmt_start(p->sema);

    // 'for'
    Parser_expect(p, TokenKind_kw_for);

    // '('
    Parser_expect(p, '(');

    // [decl_or_expr]
    StmtNode *initializer = NULL;

    if (Parser_is_decl_spec(p, Parser_current(p))) {
        // decl_or_expr
        // type_spec
        DeclSpecNode *decl_spec = Parser_parse_type_spec(p, StorageClass_none);

        // [init_declarator (',' init_declarator)*]
        Vec(DeclaratorNode) *declarators = Vec_new(DeclaratorNode)();

        if (Parser_current(p)->kind != ';') {
            // init_declarator
            Vec_push(DeclaratorNode)(
                declarators, Parser_parse_init_declarator(p, decl_spec));

            // (',' init_declarator)*
            while (Parser_current(p)->kind == ',') {
                // ','
                Parser_expect(p, ',');

                // init_declarator
                Vec_push(DeclaratorNode)(
                    declarators, Parser_parse_init_declarator(p, decl_spec));
            }
        }

        initializer = Sema_act_on_decl_stmt(p->sema, decl_spec, declarators);
    } else if (Parser_current(p)->kind != ';') {
        // comma_expr
        ExprNode *expr = Parser_parse_comma_expr(p);
        initializer = Sema_act_on_expr_stmt(p->sema, expr);
    }

    // ';'
    Parser_expect(p, ';');

    // [comma_expr]
    ExprNode *condition = NULL;

    if (Parser_current(p)->kind != ';') {
        condition = Parser_parse_comma_expr(p);
    }

    // ';'
    Parser_expect(p, ';');

    // [comma_expr]
    ExprNode *step = NULL;

    if (Parser_current(p)->kind != ')') {
        step = Parser_parse_comma_expr(p);
    }

    // ')'
    Parser_expect(p, ')');

    // stmt
    StmtNode *body = Parser_parse_stmt(p);

    return Sema_act_on_for_stmt_end(
        p->sema, initializer, condition, step, body);
}

// return_stmt:
//  'return' [comma_expr] ';'
static StmtNode *Parser_parse_return_stmt(Parser *p) {
    assert(p);

    // 'return'
    Parser_expect(p, TokenKind_kw_return);

    // [comma_expr]
    ExprNode *return_value = NULL;

    if (Parser_current(p)->kind != ';') {
        return_value = Parser_parse_comma_expr(p);
    }

    // ';'
    Parser_expect(p, ';');

    return Sema_act_on_return_stmt(p->sema, return_value);
}

// direct_declarator:
//  identifier
static DeclaratorNode *Parser_parse_direct_declarator(Parser *p) {
    assert(p);

    // TODO: direct declarator rule
    const Token *identifier = Parser_expect(p, TokenKind_identifier);

    return Sema_act_on_direct_declarator(p->sema, identifier);
}

// parameter_decl:
//  type_spec abstract_declarator
static DeclaratorNode *Parser_parse_parameter_decl(Parser *p) {
    assert(p);

    // type_spec
    DeclSpecNode *decl_spec = Parser_parse_type_spec(p, StorageClass_none);

    // TODO: abstract_declarator
    DeclaratorNode *declarator = Parser_parse_declarator(p, decl_spec);

    return Sema_act_on_parameter_decl(p->sema, declarator);
}

// array_declarator:
//  postfix_declarator '[' assign_expr ']'
static DeclaratorNode *
Parser_parse_array_declarator(Parser *p, DeclaratorNode *declarator) {
    assert(p);
    assert(declarator);

    // '['
    Parser_expect(p, '[');

    // assign_expr
    ExprNode *array_size = Parser_parse_assign_expr(p);

    // ']'
    Parser_expect(p, ']');

    return Sema_act_on_array_declarator(p->sema, declarator, array_size);
}

// function_declarator:
//  postfix_declarator '(' ')'
//  postfix_declarator '(' 'void' ')'
//  postfix_declarator '(' parameter_decl (',' parameter_decl)* [',' '...'] ')'
static DeclaratorNode *
Parser_parse_function_declarator(Parser *p, DeclaratorNode *declarator) {
    assert(p);

    Sema_act_on_function_declarator_start_of_parameter_list(p->sema);

    // '('
    Parser_expect(p, '(');

    // Parameters
    Vec(DeclaratorNode) *parameters = Vec_new(DeclaratorNode)();
    bool is_var_arg = false;

    if (Parser_current(p)->kind == ')') {
        // <empty>
        UNIMPLEMENTED();
    } else if (
        Parser_current(p)->kind == TokenKind_kw_void &&
        Parser_peek(p)->kind == ')') {
        // 'void'
        Parser_expect(p, TokenKind_kw_void);
    } else {
        // parameter_decl
        Vec_push(DeclaratorNode)(parameters, Parser_parse_parameter_decl(p));

        // (',' parameter_decl)* [',' '...']
        while (Parser_current(p)->kind == ',') {
            // ','
            Parser_expect(p, ',');

            if (Parser_current(p)->kind == TokenKind_var_arg) {
                // '...'
                Parser_expect(p, TokenKind_var_arg);

                is_var_arg = true;
                break;
            }

            // parameter_decl
            Vec_push(DeclaratorNode)(
                parameters, Parser_parse_parameter_decl(p));
        }
    }

    // ')'
    Parser_expect(p, ')');

    return Sema_act_on_function_declarator_end_of_parameter_list(
        p->sema, declarator, parameters, is_var_arg);
}

// postfix_declarator:
//  array_declarator
//  function_declarator
//  direct_declarator
static DeclaratorNode *Parser_parse_postfix_declarator(Parser *p) {
    assert(p);

    // direct_declarator
    DeclaratorNode *declarator = Parser_parse_direct_declarator(p);

    while (1) {
        switch (Parser_current(p)->kind) {
        case '[':
            // array_declarator
            declarator = Parser_parse_array_declarator(p, declarator);
            break;

        case '(':
            // function_declarator
            declarator = Parser_parse_function_declarator(p, declarator);
            break;

        default:
            return declarator;
        }
    }
}

// pointer_declarator:
//  '*' pointer_declarator
//  postfix_declarator
static DeclaratorNode *Parser_parse_pointer_declarator(Parser *p) {
    assert(p);

    if (Parser_current(p)->kind != '*') {
        // postfix_declarator
        return Parser_parse_postfix_declarator(p);
    }

    // '*'
    Parser_expect(p, '*');

    // pointer_declarator
    DeclaratorNode *declarator = Parser_parse_pointer_declarator(p);

    return Sema_act_on_pointer_declarator(p->sema, declarator);
}

// declarator:
//  pointer_declarator
static DeclaratorNode *
Parser_parse_declarator(Parser *p, DeclSpecNode *decl_spec) {
    assert(p);
    assert(decl_spec);

    // pointer_declarator
    DeclaratorNode *declarator = Parser_parse_pointer_declarator(p);

    return Sema_act_on_declarator_completed(p->sema, decl_spec, declarator);
}

// initializer:
//  compound_initializer
//  assign_expr
static ExprNode *Parser_parse_initializer(Parser *p) {
    assert(p);

    if (Parser_current(p)->kind == '{') {
        // compound_initializer
        TODO("compound_initializer");
    } else {
        // assign_expr
        return Parser_parse_assign_expr(p);
    }
}

// init_declarator:
//  declarator '=' initializer
//  declarator
static DeclaratorNode *
Parser_parse_init_declarator(Parser *p, DeclSpecNode *decl_spec) {
    assert(p);
    assert(decl_spec);

    // declarator
    DeclaratorNode *declarator = Parser_parse_declarator(p, decl_spec);

    if (Parser_current(p)->kind != '=') {
        return declarator;
    }

    // '='
    Parser_expect(p, '=');

    // initializer
    ExprNode *initializer = Parser_parse_initializer(p);

    return Sema_act_on_init_declarator(p->sema, declarator, initializer);
}

// decl_stmt:
//  decl_spec [init_declarator (',' init_declarator)*] ';'
static StmtNode *Parser_parse_decl_stmt(Parser *p) {
    assert(p);

    // decl_spec
    DeclSpecNode *decl_spec = Parser_parse_decl_spec(p);

    // [init_declarator (',' init_declarator)*]
    Vec(DeclaratorNode) *declarators = Vec_new(DeclaratorNode)();

    if (Parser_current(p)->kind != ';') {
        // init_declarator
        Vec_push(DeclaratorNode)(
            declarators, Parser_parse_init_declarator(p, decl_spec));

        // (',' init_declarator)*
        while (Parser_current(p)->kind == ',') {
            // ','
            Parser_expect(p, ',');

            // init_declarator
            Vec_push(DeclaratorNode)(
                declarators, Parser_parse_init_declarator(p, decl_spec));
        }
    }

    // ';'
    Parser_expect(p, ';');

    return Sema_act_on_decl_stmt(p->sema, decl_spec, declarators);
}

// expr_stmt:
//  comma_expr ';'
static StmtNode *Parser_parse_expr_stmt(Parser *p) {
    assert(p);

    // comma_expr
    ExprNode *expression = Parser_parse_comma_expr(p);

    // ';'
    Parser_expect(p, ';');

    return Sema_act_on_expr_stmt(p->sema, expression);
}

// stmt:
//  compound_stmt
//  if_stmt
//  while_stmt
//  do_stmt
//  for_stmt
//  switch_stmt
//  return_stmt
//  decl_stmt
//  expr_stmt
static StmtNode *Parser_parse_stmt(Parser *p) {
    assert(p);

    switch (Parser_current(p)->kind) {
    case '{':
        // compound_stmt
        return Parser_parse_compound_stmt(p);

    case TokenKind_kw_if:
        // if_stmt
        return Parser_parse_if_stmt(p);

    case TokenKind_kw_while:
        // while_stmt
        return Parser_parse_while_stmt(p);

    case TokenKind_kw_for:
        // for_stmt
        return Parser_parse_for_stmt(p);

    case TokenKind_kw_return:
        // return_stmt
        return Parser_parse_return_stmt(p);

    default:
        if (Parser_is_decl_spec(p, Parser_current(p))) {
            // decl_stmt
            return Parser_parse_decl_stmt(p);
        }

        // expr_stmt
        return Parser_parse_expr_stmt(p);
    }
}

// top_level_decl:
//  global_decl
//  function_decl
//
// function_decl:
//  decl_spec declarator compound_stmt
//
// global_decl:
//  decl_spec [init_declarator (',' init_declarator)* ] ';'
static DeclNode *Parser_parse_top_level_decl(Parser *p) {
    assert(p);

    // decl_spec
    DeclSpecNode *decl_spec = Parser_parse_decl_spec(p);

    if (Parser_current(p)->kind == ';') {
        // ';'
        Parser_consume(p);

        return Sema_act_on_global_decl(
            p->sema, decl_spec, Vec_new(DeclaratorNode)());
    }

    // init_declarator
    DeclaratorNode *declarator = Parser_parse_init_declarator(p, decl_spec);
    Symbol *symbol = DeclaratorNode_symbol(declarator);
    Vec(DeclaratorNode) *parameters = DeclaratorNode_parameters(declarator);

    bool maybe_function = declarator->kind != NodeKind_InitDeclarator &&
                          symbol->type->kind == TypeKind_function &&
                          parameters != NULL;

    if (maybe_function && Parser_current(p)->kind == '{') {
        // function_decl
        Sema_act_on_function_decl_start_of_body(p->sema, decl_spec, declarator);

        // compound_stmt
        StmtNode *body = Parser_parse_compound_stmt(p);

        return Sema_act_on_function_decl_end_of_body(
            p->sema, decl_spec, declarator, body);
    }

    // (',' init_declarator)*
    Vec(DeclaratorNode) *declarators = Vec_new(DeclaratorNode)();
    Vec_push(DeclaratorNode)(declarators, declarator);

    while (Parser_current(p)->kind == ',') {
        // ','
        Parser_expect(p, ',');

        // init_declarator
        Vec_push(DeclaratorNode)(
            declarators, Parser_parse_init_declarator(p, decl_spec));
    }

    // ';'
    Parser_expect(p, ';');

    return Sema_act_on_global_decl(p->sema, decl_spec, declarators);
}

// top_level_decls:
//  top_level_decl*
static Vec(DeclNode) * Parser_parse_top_level_decls(Parser *p) {
    assert(p);

    // top_level_decl*
    Vec(DeclNode) *declarations = Vec_new(DeclNode)();

    while (Parser_current(p)->kind != '\0') {
        // top_level_decl
        DeclNode *decl = Parser_parse_top_level_decl(p);

        Vec_push(DeclNode)(declarations, decl);
    }

    return declarations;
}

// translation_unit:
//  top_level_decls [EOF]
TranslationUnitNode *Parser_parse(Parser *p) {
    assert(p);

    // top_level_decls
    Vec(DeclNode) *declarations = Parser_parse_top_level_decls(p);

    return TranslationUnitNode_new(declarations);
}
