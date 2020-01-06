#ifndef INCLUDE_mocc_h
#define INCLUDE_mocc_h

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros
#define UNREACHABLE()                                                          \
    (fprintf(stderr, "unreachable\nat %s(%d)\n", __FILE__, __LINE__), exit(1))
#define UNIMPLEMENTED()                                                        \
    (fprintf(stderr, "unimplemented\nat %s(%d)\n", __FILE__, __LINE__), exit(1))
#define TODO(s)                                                                \
    (fprintf(stderr, "todo: " s "\nat %s(%d)\n", __FILE__, __LINE__), exit(1))

// Vec
#define Vec(T) Vec_##T
#define Vec_Element(T) Vec_Element_##T
#define Vec_new(T) Vec_new_##T
#define Vec_len(T) Vec_len_##T
#define Vec_ptr(T) Vec_ptr_##T
#define Vec_cptr(T) Vec_cptr_##T
#define Vec_get(T) Vec_get_##T
#define Vec_set(T) Vec_set_##T
#define Vec_push(T) Vec_push_##T
#define Vec_pop(T) Vec_pop_##T
#define Vec_reserve(T) Vec_reserve_##T
#define Vec_resize(T) Vec_resize_##T

#define VEC_DECL(T, X)                                                         \
    typedef struct Vec(T) Vec(T);                                              \
    typedef X Vec_Element(T);                                                  \
                                                                               \
    Vec(T) * Vec_new(T)(void);                                                 \
    size_t Vec_len(T)(const Vec(T) * v);                                       \
    Vec_Element(T) * Vec_ptr(T)(Vec(T) * v);                                   \
    const Vec_Element(T) * Vec_cptr(T)(const Vec(T) * v);                      \
    Vec_Element(T) Vec_get(T)(const Vec(T) * v, size_t i);                     \
    void Vec_set(T)(Vec(T) * v, size_t i, Vec_Element(T) x);                   \
    void Vec_push(T)(Vec(T) * v, Vec_Element(T) x);                            \
    Vec_Element(T) Vec_pop(T)(Vec(T) * v);                                     \
    void Vec_reserve(T)(Vec(T) * v, size_t cap);                               \
    void Vec_resize(T)(Vec(T) * v, size_t len);

#define VEC_DEFINE(T)                                                          \
    struct Vec(T) {                                                            \
        size_t len;                                                            \
        size_t cap;                                                            \
        Vec_Element(T) * ptr;                                                  \
    };                                                                         \
                                                                               \
    Vec(T) * Vec_new(T)(void) {                                                \
        Vec(T) *v = malloc(sizeof(*v));                                        \
        v->len = 0;                                                            \
        v->cap = 8;                                                            \
        v->ptr = malloc(sizeof(Vec_Element(T)) * v->cap);                      \
        return v;                                                              \
    }                                                                          \
                                                                               \
    size_t Vec_len(T)(const Vec(T) * v) {                                      \
        assert(v);                                                             \
        return v->len;                                                         \
    }                                                                          \
                                                                               \
    Vec_Element(T) * Vec_ptr(T)(Vec(T) * v) {                                  \
        assert(v);                                                             \
        return v->ptr;                                                         \
    }                                                                          \
                                                                               \
    const Vec_Element(T) * Vec_cptr(T)(const Vec(T) * v) {                     \
        assert(v);                                                             \
        return v->ptr;                                                         \
    }                                                                          \
                                                                               \
    Vec_Element(T) Vec_get(T)(const Vec(T) * v, size_t i) {                    \
        assert(v);                                                             \
        assert(i < v->len);                                                    \
        return v->ptr[i];                                                      \
    }                                                                          \
                                                                               \
    void Vec_set(T)(Vec(T) * v, size_t i, Vec_Element(T) x) {                  \
        assert(v);                                                             \
        assert(i < v->len);                                                    \
        v->ptr[i] = x;                                                         \
    }                                                                          \
                                                                               \
    void Vec_push(T)(Vec(T) * v, Vec_Element(T) x) {                           \
        assert(v);                                                             \
        if (v->len == v->cap) {                                                \
            Vec_reserve(T)(v, v->cap * 2);                                     \
        }                                                                      \
        v->ptr[v->len++] = x;                                                  \
    }                                                                          \
                                                                               \
    Vec_Element(T) Vec_pop(T)(Vec(T) * v) {                                    \
        assert(v);                                                             \
        assert(v->len > 0);                                                    \
        return v->ptr[--v->len];                                               \
    }                                                                          \
                                                                               \
    void Vec_reserve(T)(Vec(T) * v, size_t cap) {                              \
        assert(v);                                                             \
        if (cap > v->cap) {                                                    \
            v->cap = cap;                                                      \
            v->ptr = realloc(v->ptr, sizeof(Vec_Element(T)) * v->cap);         \
        }                                                                      \
    }                                                                          \
                                                                               \
    void Vec_resize(T)(Vec(T) * v, size_t len) {                               \
        assert(v);                                                             \
        Vec_reserve(T)(v, len);                                                \
        v->len = len;                                                          \
    }

VEC_DECL(Token, struct Token *)
VEC_DECL(ExprNode, struct ExprNode *)
VEC_DECL(StmtNode, struct StmtNode *)
VEC_DECL(DeclaratorNode, struct DeclaratorNode *)
VEC_DECL(DeclNode, struct DeclNode *)

// File
char *File_read(const char *path);

// Token
typedef int TokenKind;

enum {
    TokenKind_unused = 256,
#define TOKEN(name, text) TokenKind_##name,
#include "Token.def"
};

typedef struct Token {
    TokenKind kind;
    char *text; // For identifer and number
} Token;

// Ast
typedef enum NodeKind {
#define NODE(name, base) NodeKind_##name##base,
#include "Ast.def"
} NodeKind;

typedef struct Node Node;
typedef struct ExprNode ExprNode;
typedef struct StmtNode StmtNode;
typedef struct DeclaratorNode DeclaratorNode;
typedef struct DeclNode DeclNode;

#define NODE(name, base) typedef struct name##base##Node name##base##Node;
#include "Ast.def"

struct Node {
    NodeKind kind;
};
struct ExprNode {
    NodeKind kind;
};
struct StmtNode {
    NodeKind kind;
};
struct DeclaratorNode {
    NodeKind kind;
};
struct DeclNode {
    NodeKind kind;
};

#define NODE(name, base)                                                       \
    Node *name##base##Node_base_node(name##base##Node *p);                     \
    const Node *name##base##Node_cbase_node(name##base##Node *p);              \
    base##Node *name##base##Node_base(name##base##Node *p);                    \
    const base##Node *name##base##Node_cbase(const name##base##Node *p);       \
    name##base##Node *name##base##Node_cast_node(Node *p);                     \
    const name##base##Node *name##base##Node_ccast_node(const Node *p);        \
    name##base##Node *name##base##Node_cast(base##Node *p);                    \
    const name##base##Node *name##base##Node_ccast(const base##Node *p);       \
                                                                               \
    struct name##base##Node {                                                  \
        NODE_MEMBER(NodeKind, kind)
#define NODE_MEMBER(T, x) T x;
#define NODE_END()                                                             \
    }                                                                          \
    ;
#include "Ast.def"

IdentifierExprNode *IdentifierExprNode_new(const char *name);
IntegerExprNode *IntegerExprNode_new(long long value);
AssignExprNode *AssignExprNode_new(ExprNode *lhs, ExprNode *rhs);

CompoundStmtNode *CompoundStmtNode_new(Vec(StmtNode) * statements);
ReturnStmtNode *ReturnStmtNode_new(ExprNode *return_value);
DeclStmtNode *DeclStmtNode_new(Vec(DeclaratorNode) * declarators);
ExprStmtNode *ExprStmtNode_new(ExprNode *expression);

DirectDeclaratorNode *DirectDeclaratorNode_new(const char *name);

FunctionDeclNode *FunctionDeclNode_new(const char *name, StmtNode *body);

TranslationUnitNode *TranslationUnitNode_new(Vec(DeclNode) * declarations);

void Node_dump(const Node *p, FILE *fp);

// Lexer
typedef struct Lexer Lexer;

Lexer *Lexer_new(const char *filename, const char *text);
Token *Lexer_read(Lexer *l);

// Preprocessor
Vec(Token) * Preprocessor_read(const char *filename, const char *text);

// Parser
typedef struct Parser Parser;

Parser *Parser_new(const Vec(Token) * tokens);
TranslationUnitNode *Parser_parse(Parser *p);

// CodeGen
void CodeGen_gen(TranslationUnitNode *p, FILE *fp);

// Tests
#define ERROR(...) (fprintf(stderr, __VA_ARGS__), exit(1))

void test_Vec(void);
void test_File(void);
void test_Ast(void);
void test_Lexer(void);
void test_Preprocessor(void);
void test_Parser(void);

void check_Node_dump(
    const char *test_name, const Node *p, const char *expected);

#endif // INCLUDE_mocc_h
