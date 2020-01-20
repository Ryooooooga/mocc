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
    case TokenKind_kw_int:
    case TokenKind_kw_char:
        return true;

    case TokenKind_identifier:
        (void)p; // TODO: typedef name
        return false;

    default:
        return false;
    }
}

static ExprNode *Parser_parse_assign_expr(Parser *p);
static ExprNode *Parser_parse_comma_expr(Parser *p);
static StmtNode *Parser_parse_stmt(Parser *p);
static DeclaratorNode *
Parser_parse_declarator(Parser *p, DeclSpecNode *decl_spec);

// type_spec:
//  type
static DeclSpecNode *Parser_parse_type_spec(Parser *p) {
    assert(p);

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

    case TokenKind_identifier:
        TODO("typedef name");
        break;

    default:
        UNIMPLEMENTED();
        break;
    }

    return Sema_act_on_decl_spec(p->sema, base_type);
}

// decl_spec:
//  type_spec
static DeclSpecNode *Parser_parse_decl_spec(Parser *p) {
    assert(p);

    return Parser_parse_type_spec(p);
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

// primary_expr:
//  identifier_expr
//  number_expr
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

// postfix_expr:
//  subscript_expr
//  call_expr
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

        default:
            return node;
        }
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
    case '*':
    case TokenKind_kw_sizeof: {
        // unary_op
        const Token *operator= Parser_consume(p);

        // unary_expr
        ExprNode *operand = Parser_parse_unary_expr(p);

        return Sema_act_on_unary_expr(p->sema, operator, operand);
    }

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

    // TODO: multiplicative_expr rule
    return Parser_parse_unary_expr(p);
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

// conditional_expr:
//  TODO: conditional_expr rule
static ExprNode *Parser_parse_conditional_expr(Parser *p) {
    assert(p);

    // TODO: conditional_expr rule
    return Parser_parse_additive_expr(p);
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
    DeclSpecNode *decl_spec = Parser_parse_type_spec(p);

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

            // TODO: ...
            // parameter_decl
            Vec_push(DeclaratorNode)(
                parameters, Parser_parse_parameter_decl(p));
        }
    }

    // ')'
    Parser_expect(p, ')');

    return Sema_act_on_function_declarator_end_of_parameter_list(
        p->sema, declarator, parameters);
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
//  function_decl
//
// function_decl:
//  decl_spec declarator compound_stmt
static DeclNode *Parser_parse_top_level_decl(Parser *p) {
    assert(p);

    // decl_spec
    DeclSpecNode *decl_spec = Parser_parse_decl_spec(p);

    // declarator
    DeclaratorNode *declarator = Parser_parse_declarator(p, decl_spec);

    Sema_act_on_function_decl_start_of_body(p->sema, decl_spec, declarator);

    // compound_stmt
    StmtNode *body = Parser_parse_compound_stmt(p);

    return Sema_act_on_function_decl_end_of_body(
        p->sema, decl_spec, declarator, body);
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
