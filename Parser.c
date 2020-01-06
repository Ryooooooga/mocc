#include "mocc.h"

struct Parser {
    const Vec(Token) * tokens;
    size_t cursor;
};

Parser *Parser_new(const Vec(Token) * tokens) {
    assert(tokens);

    Parser *p = malloc(sizeof(Parser));
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

static ExprNode *Parser_parse_expr(Parser *p);
static StmtNode *Parser_parse_stmt(Parser *p);

// number_expr:
//  number
static ExprNode *Parser_parse_number_expr(Parser *p) {
    assert(p);

    // number
    const Token *num = Parser_expect(p, TokenKind_number);
    long long value = atoll(num->text); // TODO: conversion

    return IntegerExprNode_base(IntegerExprNode_new(value));
}

// primary_expr:
//  TODO: primary_expr rule
static ExprNode *Parser_parse_primary_expr(Parser *p) {
    assert(p);

    // TODO: primary_expr rule
    return Parser_parse_number_expr(p);
}

// expr:
//  TODO: expr rule
static ExprNode *Parser_parse_expr(Parser *p) {
    assert(p);

    // TODO: expr rule
    return Parser_parse_primary_expr(p);
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
//  'return' expr? ';'
static StmtNode *Parser_parse_return_stmt(Parser *p) {
    assert(p);

    // 'return'
    Parser_expect(p, TokenKind_kw_return);

    // expr?
    ExprNode *return_value = NULL;

    if (Parser_peek(p)->kind != ';') {
        return_value = Parser_parse_expr(p);
    }

    // ';'
    Parser_expect(p, ';');

    return ReturnStmtNode_base(ReturnStmtNode_new(return_value));
}

// stmt:
//  compound_stmt
//  return_stmt
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
        // expr_stmt
        TODO("expr_stmt");
    }
}

// top_level_decl:
//  function_decl
//
// function_decl:
//  type identifier params compound_stmt
static DeclNode *Parser_parse_top_level_decl(Parser *p) {
    assert(p);

    // type
    Parser_expect(p, TokenKind_kw_int);

    // identifier
    const Token *ident = Parser_expect(p, TokenKind_identifier);

    // params
    Parser_expect(p, '(');
    Parser_expect(p, TokenKind_kw_void);
    Parser_expect(p, ')');

    // compound_stmt
    StmtNode *body = Parser_parse_compound_stmt(p);

    return FunctionDeclNode_base(FunctionDeclNode_new(ident->text, body));
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
