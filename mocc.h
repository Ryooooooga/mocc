#ifndef INCLUDE_mocc_h
#define INCLUDE_mocc_h

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <ctype.h>
#include <stdalign.h>
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

VEC_DECL(String, const char *)
VEC_DECL(size_t, size_t)
VEC_DECL(Token, struct Token *)
VEC_DECL(Macro, struct Macro *)
VEC_DECL(Type, struct Type *)
VEC_DECL(Symbol, struct Symbol *)
VEC_DECL(ExprNode, struct ExprNode *)
VEC_DECL(StmtNode, struct StmtNode *)
VEC_DECL(DeclaratorNode, struct DeclaratorNode *)
VEC_DECL(DeclNode, struct DeclNode *)
VEC_DECL(MemberDeclNode, struct MemberDeclNode *)
VEC_DECL(EnumeratorDeclNode, struct EnumeratorDeclNode *)

// Path
char *Path_join(const char *dir, const char *rel_path);

// File
char *File_read(const char *path);

// Type
typedef enum ValueCategory {
    ValueCategory_lvalue,
    ValueCategory_rvalue,
} ValueCategory;

typedef enum TypeKind {
    TypeKind_void,
    TypeKind_char,
    TypeKind_int,
    TypeKind_pointer,
    TypeKind_function,
    TypeKind_array,
    TypeKind_struct,
    TypeKind_enum,
} TypeKind;

typedef struct Type Type;

struct Type {
    TypeKind kind;

    // For pointer type
    Type *pointee_type;

    // For function type
    Type *return_type;
    Vec(Type) * parameter_types;
    bool is_var_arg;

    // For array type
    Type *element_type;
    size_t array_length;

    // For struct type
    struct Symbol *struct_symbol;
    Vec(MemberDeclNode) * member_decls;
    Vec(Symbol) * member_symbols;

    // For struct type
    Type *underlying_type;
    struct Symbol *enum_symbol;
    Vec(EnumeratorDeclNode) * enumerators;
};

Type *VoidType_new(void);
Type *CharType_new(void);
Type *IntType_new(void);

Type *PointerType_new(Type *pointee_type);
Type *PointerType_pointee_type(const Type *pointer_type);

Type *FunctionType_new(
    Type *return_type, Vec(Type) * parameter_types, bool is_var_arg);
Type *FunctionType_return_type(const Type *function_type);
Vec(Type) * FunctionType_parameter_types(const Type *function_type);
bool FunctionType_is_var_arg(const Type *function_type);

Type *ArrayType_new(Type *element_type, size_t array_length);
Type *ArrayType_element_type(const Type *array_type);
size_t ArrayType_length(const Type *array_type);

Type *StructType_new(void);
bool StructType_is_defined(const Type *struct_type);
struct Symbol *
StructType_find_member(const Type *struct_type, const char *member_name);

Type *
EnumType_new(Type *underlying_type, Vec(EnumeratorDeclNode) * enumerators);

size_t Type_sizeof(const Type *type);
size_t Type_alignof(const Type *type);

bool Type_equals(const Type *a, const Type *b);
bool Type_is_scalar(const Type *type);
bool Type_is_incomplete_type(const Type *type);
bool Type_is_function_pointer_type(const Type *type);

// Token
typedef int TokenKind;

enum {
    TokenKind_unused = 256,
#define TOKEN(name, text) TokenKind_##name,
#include "Token.def"
};

typedef struct Token {
    TokenKind kind;
    char *text;
    char *string; // For string
    size_t string_len;
    bool is_bol;
    bool has_spaces;

    Vec(String) * hidden_set;
} Token;

// Symbol
typedef enum StorageClass {
    StorageClass_none,
    StorageClass_typedef,
    StorageClass_enum,
} StorageClass;

typedef struct Symbol {
    const char *name;
    StorageClass storage_class;
    Type *type;

    // For enumerators
    int enum_value;

    // For functions
    bool has_body;

    // For CodeGen
    struct NativeAddress *address;
} Symbol;

Symbol *Symbol_new(const char *name, StorageClass storage_class, Type *type);

// Scope
typedef struct Scope Scope;

Scope *Scope_new(Scope *parent_scope);
Scope *Scope_parent_scope(const Scope *s);

Symbol *Scope_find(Scope *s, const char *name, bool recursive);
bool Scope_try_register(Scope *s, Symbol *symbol);

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
    Type *result_type;
    ValueCategory value_category;
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

typedef enum UnaryOp {
#define UNARY_OP(name) UnaryOp_##name,
#include "Ast.def"
} UnaryOp;

typedef enum BinaryOp {
#define BINARY_OP(name) BinaryOp_##name,
#include "Ast.def"
} BinaryOp;

typedef enum ImplicitCastOp {
    ImplicitCastOp_lvalue_to_rvalue,
    ImplicitCastOp_function_to_function_pointer,
    ImplicitCastOp_array_to_pointer,
    ImplicitCastOp_integral_cast,
} ImplicitCastOp;

// clang-format off
#define NODE(name, base)                                                       \
    Node *name##base##Node_base_node(name##base##Node *p);                     \
    const Node *name##base##Node_cbase_node(const name##base##Node *p);        \
    base##Node *name##base##Node_base(name##base##Node *p);                    \
    const base##Node *name##base##Node_cbase(const name##base##Node *p);       \
    name##base##Node *name##base##Node_cast_node(Node *p);                     \
    const name##base##Node *name##base##Node_ccast_node(const Node *p);        \
    name##base##Node *name##base##Node_cast(base##Node *p);                    \
    const name##base##Node *name##base##Node_ccast(const base##Node *p);       \
                                                                               \
    struct name##base##Node {                                                  \
        NODE_MEMBER(NodeKind, kind)

#define EXPR_NODE(name)                                                        \
    NODE(name, Expr)                                                           \
        NODE_MEMBER(Type *, result_type)                                       \
        NODE_MEMBER(ValueCategory, value_category)                             \

#define NODE_MEMBER(T, x)                                                      \
        T x;

#define NODE_END()                                                             \
    };
#include "Ast.def"
// clang-format on

// Expressions
IdentifierExprNode *IdentifierExprNode_new(
    Type *result_type, ValueCategory value_category, Symbol *symbol);

EnumeratorExprNode *EnumeratorExprNode_new(
    Type *result_type, ValueCategory value_category, Symbol *symbol, int value);

IntegerExprNode *IntegerExprNode_new(
    Type *result_type, ValueCategory value_category, long long value);

StringExprNode *StringExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    const char *string,
    size_t length);

SubscriptExprNode *SubscriptExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *array,
    ExprNode *index);

CallExprNode *CallExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *callee,
    Vec(ExprNode) * arguments);

DotExprNode *DotExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *parent,
    Symbol *member_symbol);

ArrowExprNode *ArrowExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *parent,
    Symbol *member_symbol);

UnaryExprNode *UnaryExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    UnaryOp
    operator,
    ExprNode *operand);

BinaryExprNode *BinaryExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    BinaryOp
    operator,
    ExprNode *lhs,
    ExprNode *rhs);

AssignExprNode *AssignExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *lhs,
    ExprNode *rhs);

ImplicitCastExprNode *ImplicitCastExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ImplicitCastOp
    operator,
    ExprNode *expression);

// Statements
CompoundStmtNode *CompoundStmtNode_new(Vec(StmtNode) * statements);

IfStmtNode *
IfStmtNode_new(ExprNode *condition, StmtNode *if_true, StmtNode *if_false);

WhileStmtNode *WhileStmtNode_new(ExprNode *condition, StmtNode *body);

ForStmtNode *ForStmtNode_new(
    StmtNode *initializer, ExprNode *condition, ExprNode *step, StmtNode *body);

ReturnStmtNode *ReturnStmtNode_new(ExprNode *return_value);

DeclStmtNode *
DeclStmtNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

ExprStmtNode *ExprStmtNode_new(ExprNode *expression);

// Declarators
DirectDeclaratorNode *DirectDeclaratorNode_new(const char *name);

ArrayDeclaratorNode *
ArrayDeclaratorNode_new(DeclaratorNode *declarator, ExprNode *array_size);

FunctionDeclaratorNode *FunctionDeclaratorNode_new(
    DeclaratorNode *declarator,
    Vec(DeclaratorNode) * parameters,
    bool is_var_arg);

PointerDeclaratorNode *PointerDeclaratorNode_new(DeclaratorNode *declarator);

InitDeclaratorNode *
InitDeclaratorNode_new(DeclaratorNode *declarator, ExprNode *initializer);

Symbol *DeclaratorNode_symbol(const DeclaratorNode *p);
Vec(DeclaratorNode) * DeclaratorNode_parameters(const DeclaratorNode *p);

// Declarations
GlobalDeclNode *
GlobalDeclNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

FunctionDeclNode *FunctionDeclNode_new(
    DeclSpecNode *decl_spec,
    DeclaratorNode *declarator,
    StmtNode *body,
    Vec(Symbol) * local_variables);

MemberDeclNode *
MemberDeclNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

EnumeratorDeclNode *EnumeratorDeclNode_new(Symbol *symbol, ExprNode *value);

// Miscs
DeclSpecNode *DeclSpecNode_new(StorageClass storage_class, Type *base_type);

TranslationUnitNode *TranslationUnitNode_new(Vec(DeclNode) * declarations);

void Node_dump(const Node *p, FILE *fp);

// Lexer
typedef struct Lexer Lexer;

Lexer *Lexer_new(const char *filename, const char *text);
Token *Lexer_read(Lexer *l);

// Preprocessor
typedef struct Macro {
    const char *name;
    Vec(String) * parameters;
    Vec(Token) * contents;
} Macro;

Macro *Macro_new_simple(const char *name, Vec(Token) * contents);
bool Macro_is_function(const Macro *m);

Vec(Token) * Preprocessor_read(const char *filename, const char *text);

// Parser
typedef struct Parser Parser;

Parser *Parser_new(const Vec(Token) * tokens);
TranslationUnitNode *Parser_parse(Parser *p);

// Sema
typedef struct Sema Sema;

Sema *Sema_new(void);

// Types
Type *Sema_act_on_struct_type_reference(Sema *s, const Token *identifier);
Type *
Sema_act_on_struct_type_start_of_member_list(Sema *s, const Token *identifier);
Type *Sema_act_on_struct_type_end_of_member_list(
    Sema *s, Type *struct_type, Vec(MemberDeclNode) * member_decls);

MemberDeclNode *Sema_act_on_struct_type_member_decl(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

Type *Sema_act_on_enum_type_reference(Sema *s, const Token *identifier);
void Sema_act_on_enum_type_start_of_list(Sema *s, const Token *identifier);
Type *Sema_act_on_enum_type_end_of_list(
    Sema *s, const Token *identifier, Vec(EnumeratorDeclNode) * enumerators);

EnumeratorDeclNode *
Sema_act_on_enumerator(Sema *s, const Token *identifier, ExprNode *value);

bool Sema_is_typedef_name(Sema *s, const Token *identifier);
Type *Sema_act_on_typedef_name(Sema *s, const Token *identifier);

// Expressions
ExprNode *Sema_act_on_identifier_expr(Sema *s, const Token *identifier);

ExprNode *Sema_act_on_integer_expr(Sema *s, const Token *integer);

ExprNode *Sema_act_on_string_expr(Sema *s, const Token *string);

ExprNode *Sema_act_on_subscript_expr(Sema *s, ExprNode *array, ExprNode *index);

ExprNode *
Sema_act_on_call_expr(Sema *s, ExprNode *callee, Vec(ExprNode) * arguments);

ExprNode *
Sema_act_on_dot_expr(Sema *s, ExprNode *parent, const Token *identifier);

ExprNode *
Sema_act_on_arrow_expr(Sema *s, ExprNode *parent, const Token *identifier);

ExprNode *
Sema_act_on_unary_expr(Sema *s, const Token *operator, ExprNode *operand);

ExprNode *Sema_act_on_binary_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs);

ExprNode *Sema_act_on_assign_expr(
    Sema *s, ExprNode *lhs, const Token *operator, ExprNode *rhs);

// Statements
StmtNode *Sema_act_on_if_stmt(
    Sema *s, ExprNode *condition, StmtNode *if_true, StmtNode *if_false);

StmtNode *Sema_act_on_while_stmt(Sema *s, ExprNode *condition, StmtNode *body);

void Sema_act_on_for_stmt_start(Sema *s);
StmtNode *Sema_act_on_for_stmt_end(
    Sema *s,
    StmtNode *initializer,
    ExprNode *condition,
    ExprNode *step,
    StmtNode *body);

StmtNode *Sema_act_on_return_stmt(Sema *s, ExprNode *return_value);

StmtNode *Sema_act_on_decl_stmt(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

StmtNode *Sema_act_on_expr_stmt(Sema *s, ExprNode *expression);

// Declarators
DeclaratorNode *Sema_act_on_direct_declarator(Sema *s, const Token *identifier);

DeclaratorNode *Sema_act_on_array_declarator(
    Sema *s, DeclaratorNode *declarator, ExprNode *array_size);

void Sema_act_on_function_declarator_start_of_parameter_list(Sema *s);
DeclaratorNode *Sema_act_on_function_declarator_end_of_parameter_list(
    Sema *s,
    DeclaratorNode *declarator,
    Vec(DeclaratorNode) * parameters,
    bool is_var_arg);

DeclaratorNode *
Sema_act_on_pointer_declarator(Sema *s, DeclaratorNode *declarator);

DeclaratorNode *Sema_act_on_declarator_completed(
    Sema *s, DeclSpecNode *decl_spec, DeclaratorNode *declarator);

DeclaratorNode *Sema_act_on_init_declarator(
    Sema *s, DeclaratorNode *declarator, ExprNode *initializer);

// Declarations
DeclSpecNode *
Sema_act_on_decl_spec(Sema *s, StorageClass storage_class, Type *base_type);

DeclaratorNode *Sema_act_on_parameter_decl(Sema *s, DeclaratorNode *declarator);

DeclNode *Sema_act_on_global_decl(
    Sema *s, DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators);

void Sema_act_on_function_decl_start_of_body(
    Sema *s, DeclSpecNode *decl_spec, DeclaratorNode *declarator);
DeclNode *Sema_act_on_function_decl_end_of_body(
    Sema *s,
    DeclSpecNode *decl_spec,
    DeclaratorNode *declarator,
    StmtNode *body);

// CodeGen
void CodeGen_gen(TranslationUnitNode *p, FILE *fp);

// Tests
#define ERROR(...) (fprintf(stderr, __VA_ARGS__), exit(1))

void test_Vec(void);
void test_Path(void);
void test_File(void);
void test_Ast(void);
void test_Lexer(void);
void test_Preprocessor(void);
void test_Parser(void);

void check_Node_dump(
    const char *test_name, const Node *p, const char *expected);

#endif // INCLUDE_mocc_h
