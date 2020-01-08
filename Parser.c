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

static const Token *Parser_peek(const Parser *p) {
    assert(p);

    return Vec_get(Token)(p->tokens, p->cursor);
}

static const Token *Parser_consume(Parser *p) {
    assert(p);
    assert(Parser_peek(p)->kind != '\0');

    return Vec_get(Token)(p->tokens, p->cursor++);
}

static const Token *Parser_expect(Parser *p, TokenKind kind) {
    assert(p);

    const Token *t = Parser_peek(p);
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
        return true;

    case TokenKind_identifier:
        (void)p; // TODO: typedef name
        return false;

    default:
        return false;
    }
}

static ExprNode *Parser_parse_comma_expr(Parser *p);
static StmtNode *Parser_parse_stmt(Parser *p);

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

    const Token *t = Parser_peek(p);
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

// call_expr:
//  postfix_expr '(' [assign_expr (',' assign_expr)*] ')'
static ExprNode *Parser_parse_call_expr(Parser *p, ExprNode *callee) {
    assert(p);
    assert(callee);

    // '('
    Parser_expect(p, '(');

    // ')'
    Parser_expect(p, ')');

    return Sema_act_on_call_expr(p->sema, callee);
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
        switch (Parser_peek(p)->kind) {
        case '[':
            UNIMPLEMENTED();
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

    switch (Parser_peek(p)->kind) {
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

    while (Parser_peek(p)->kind == '+' || Parser_peek(p)->kind == '-') {
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

    if (Parser_peek(p)->kind != '=') {
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

    while (Parser_peek(p)->kind != '}') {
        StmtNode *stmt = Parser_parse_stmt(p);

        Vec_push(StmtNode)(statements, stmt);
    }

    // '}'
    Parser_expect(p, '}');

    return CompoundStmtNode_base(CompoundStmtNode_new(statements));
}

// return_stmt:
//  'return' [comma_expr] ';'
static StmtNode *Parser_parse_return_stmt(Parser *p) {
    assert(p);

    // 'return'
    Parser_expect(p, TokenKind_kw_return);

    // [expr]
    ExprNode *return_value = NULL;

    if (Parser_peek(p)->kind != ';') {
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

// pointer_declarator:
//  '*' pointer_declarator
//  direct_declarator
static DeclaratorNode *Parser_parse_pointer_declarator(Parser *p) {
    assert(p);

    if (Parser_peek(p)->kind != '*') {
        // direct_declarator
        return Parser_parse_direct_declarator(p);
    }

    // '*'
    Parser_expect(p, '*');

    // pointer_declarator
    DeclaratorNode *declarator = Parser_parse_pointer_declarator(p);

    return Sema_act_on_pointer_declarator(p->sema, declarator);
}

// declarator:
//  pointer_declarator
static DeclaratorNode *Parser_parse_declarator(Parser *p) {
    assert(p);

    // pointer_declarator
    DeclaratorNode *declarator = Parser_parse_pointer_declarator(p);

    return Sema_act_on_declarator_completed(p->sema, declarator);
}

// initializer:
//  compound_initializer
//  assign_expr
static ExprNode *Parser_parse_initializer(Parser *p) {
    assert(p);

    if (Parser_peek(p)->kind == '{') {
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
static DeclaratorNode *Parser_parse_init_declarator(Parser *p) {
    assert(p);

    // declarator
    DeclaratorNode *declarator = Parser_parse_declarator(p);

    if (Parser_peek(p)->kind != '=') {
        return declarator;
    }

    // '='
    Parser_expect(p, '=');

    // initializer
    ExprNode *initializer = Parser_parse_initializer(p);

    return Sema_act_on_init_declarator(p->sema, declarator, initializer);
}

// decl_stmt:
//  type [init_declarator (',' init_declarator)*] ';'
static StmtNode *Parser_parse_decl_stmt(Parser *p) {
    assert(p);

    // type
    Parser_expect(p, TokenKind_kw_int); // TODO: type

    // [init_declarator (',' init_declarator)*]
    Vec(DeclaratorNode) *declarators = Vec_new(DeclaratorNode)();

    if (Parser_peek(p)->kind != ';') {
        DeclaratorNode *declarator = Parser_parse_init_declarator(p);

        Vec_push(DeclaratorNode)(declarators, declarator);
    }

    // ';'
    Parser_expect(p, ';');

    return Sema_act_on_decl_stmt(p->sema, declarators);
}

// expr_stmt:
//  comma_expr ';'
static StmtNode *Parser_parse_expr_stmt(Parser *p) {
    assert(p);

    // comma_expr
    ExprNode *expression = Parser_parse_comma_expr(p);

    // ';'
    Parser_expect(p, ';');

    return ExprStmtNode_base(ExprStmtNode_new(expression));
}

// stmt:
//  compound_stmt
//  return_stmt
//  decl_stmt
//  expr_stmt
static StmtNode *Parser_parse_stmt(Parser *p) {
    assert(p);

    switch (Parser_peek(p)->kind) {
    case '{':
        // compound_stmt
        return Parser_parse_compound_stmt(p);

    case TokenKind_kw_return:
        // return_stmt
        return Parser_parse_return_stmt(p);

    default:
        if (Parser_is_decl_spec(p, Parser_peek(p))) {
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
//  type declarator compound_stmt
static DeclNode *Parser_parse_top_level_decl(Parser *p) {
    assert(p);

    // type
    Parser_expect(p, TokenKind_kw_int);

    // declarator
    DeclaratorNode *declarator = Parser_parse_declarator(p);

    // TODO: function declarator
    // params
    DeclaratorNode_symbol(declarator)->type =
        FunctionType_new(DeclaratorNode_symbol(declarator)->type);
    Parser_expect(p, '(');
    Parser_expect(p, TokenKind_kw_void);
    Parser_expect(p, ')');

    Sema_act_on_function_decl_start_of_body(p->sema, declarator);

    // compound_stmt
    StmtNode *body = Parser_parse_compound_stmt(p);

    return Sema_act_on_function_decl_end_of_body(p->sema, declarator, body);
}

// top_level_decls:
//  top_level_decl*
static Vec(DeclNode) * Parser_parse_top_level_decls(Parser *p) {
    assert(p);

    // top_level_decl*
    Vec(DeclNode) *declarations = Vec_new(DeclNode)();

    while (Parser_peek(p)->kind != '\0') {
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
