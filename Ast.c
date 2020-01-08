#include "mocc.h"

#define NODE(name, base)                                                       \
    Node *name##base##Node_base_node(name##base##Node *p) {                    \
        assert(p);                                                             \
        return (Node *)p;                                                      \
    }                                                                          \
    const Node *name##base##Node_cbase_node(name##base##Node *p) {             \
        assert(p);                                                             \
        return (const Node *)p;                                                \
    }                                                                          \
    base##Node *name##base##Node_base(name##base##Node *p) {                   \
        assert(p);                                                             \
        return (base##Node *)p;                                                \
    }                                                                          \
    const base##Node *name##base##Node_cbase(const name##base##Node *p) {      \
        assert(p);                                                             \
        return (const base##Node *)p;                                          \
    }                                                                          \
    name##base##Node *name##base##Node_cast_node(Node *p) {                    \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (name##base##Node *)p;                                          \
    }                                                                          \
    const name##base##Node *name##base##Node_ccast_node(const Node *p) {       \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (const name##base##Node *)p;                                    \
    }                                                                          \
    name##base##Node *name##base##Node_cast(base##Node *p) {                   \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (name##base##Node *)p;                                          \
    }                                                                          \
    const name##base##Node *name##base##Node_ccast(const base##Node *p) {      \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (const name##base##Node *)p;                                    \
    }
#include "Ast.def"

#define NODE(name, base)                                                       \
    static name##base##Node *name##base##Node_alloc(void) {                    \
        name##base##Node *p = malloc(sizeof(*p));                              \
        p->kind = NodeKind_##name##base;                                       \
        return p;                                                              \
    }

#define EXPR_NODE(name)                                                        \
    static name##ExprNode *name##ExprNode_alloc(                               \
        Type *result_type, ValueCategory value_category) {                     \
        assert(result_type);                                                   \
        name##ExprNode *p = malloc(sizeof(*p));                                \
        p->kind = NodeKind_##name##Expr;                                       \
        p->result_type = result_type;                                          \
        p->value_category = value_category;                                    \
        return p;                                                              \
    }
#include "Ast.def"

// Expressions
IdentifierExprNode *IdentifierExprNode_new(
    Type *result_type, ValueCategory value_category, Symbol *symbol) {
    assert(symbol);

    IdentifierExprNode *p =
        IdentifierExprNode_alloc(result_type, value_category);
    p->symbol = symbol;

    return p;
}

IntegerExprNode *IntegerExprNode_new(
    Type *result_type, ValueCategory value_category, long long value) {
    IntegerExprNode *p = IntegerExprNode_alloc(result_type, value_category);
    p->value = value;

    return p;
}

CallExprNode *CallExprNode_new(
    Type *result_type, ValueCategory value_category, ExprNode *callee) {
    assert(callee);

    CallExprNode *p = CallExprNode_alloc(result_type, value_category);
    p->callee = callee;

    return p;
}

UnaryExprNode *UnaryExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    UnaryOp
    operator,
    ExprNode *operand) {
    assert(operand);

    UnaryExprNode *p = UnaryExprNode_alloc(result_type, value_category);
    p->operator= operator;
    p->operand = operand;

    return p;
}

BinaryExprNode *BinaryExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    BinaryOp
    operator,
    ExprNode *lhs,
    ExprNode *rhs) {
    assert(lhs);
    assert(rhs);

    BinaryExprNode *p = BinaryExprNode_alloc(result_type, value_category);
    p->operator= operator;
    p->lhs = lhs;
    p->rhs = rhs;

    return p;
}

AssignExprNode *AssignExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *lhs,
    ExprNode *rhs) {
    assert(lhs);
    assert(rhs);

    AssignExprNode *p = AssignExprNode_alloc(result_type, value_category);
    p->lhs = lhs;
    p->rhs = rhs;

    return p;
}

ImplicitCastExprNode *ImplicitCastExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ImplicitCastOp
    operator,
    ExprNode *expression) {
    assert(expression);

    ImplicitCastExprNode *p =
        ImplicitCastExprNode_alloc(result_type, value_category);
    p->operator= operator;
    p->expression = expression;

    return p;
}

// Statements
CompoundStmtNode *CompoundStmtNode_new(Vec(StmtNode) * statements) {
    assert(statements);

    CompoundStmtNode *p = CompoundStmtNode_alloc();
    p->statements = statements;

    return p;
}

ReturnStmtNode *ReturnStmtNode_new(ExprNode *return_value) {
    ReturnStmtNode *p = ReturnStmtNode_alloc();
    p->return_value = return_value;

    return p;
}

DeclStmtNode *DeclStmtNode_new(Vec(DeclaratorNode) * declarators) {
    assert(declarators);

    DeclStmtNode *p = DeclStmtNode_alloc();
    p->declarators = declarators;

    return p;
}

ExprStmtNode *ExprStmtNode_new(ExprNode *expression) {
    assert(expression);

    ExprStmtNode *p = ExprStmtNode_alloc();
    p->expression = expression;

    return p;
}

// Declarators
DirectDeclaratorNode *DirectDeclaratorNode_new(Symbol *symbol) {
    assert(symbol);

    DirectDeclaratorNode *p = DirectDeclaratorNode_alloc();
    p->symbol = symbol;

    return p;
}

static Symbol *DirectDeclaratorNode_symbol(const DirectDeclaratorNode *p) {
    assert(p);

    return p->symbol;
}

FunctionDeclaratorNode *FunctionDeclaratorNode_new(DeclaratorNode *declarator) {
    assert(declarator);

    FunctionDeclaratorNode *p = FunctionDeclaratorNode_alloc();
    p->declarator = declarator;

    return p;
}

static Symbol *FunctionDeclaratorNode_symbol(const FunctionDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_symbol(p->declarator);
}

PointerDeclaratorNode *PointerDeclaratorNode_new(DeclaratorNode *declarator) {
    assert(declarator);

    PointerDeclaratorNode *p = PointerDeclaratorNode_alloc();
    p->declarator = declarator;

    return p;
}

static Symbol *PointerDeclaratorNode_symbol(const PointerDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_symbol(p->declarator);
}

InitDeclaratorNode *
InitDeclaratorNode_new(DeclaratorNode *declarator, ExprNode *initializer) {
    assert(declarator);
    assert(initializer);

    InitDeclaratorNode *p = InitDeclaratorNode_alloc();
    p->declarator = declarator;
    p->initializer = initializer;

    return p;
}

static Symbol *InitDeclaratorNode_symbol(const InitDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_symbol(p->declarator);
}

Symbol *DeclaratorNode_symbol(const DeclaratorNode *p) {
    assert(p);

    switch (p->kind) {
#define DECLARATOR_NODE(name)                                                  \
    case NodeKind_##name##Declarator:                                          \
        return name##DeclaratorNode_symbol(name##DeclaratorNode_ccast(p));
#include "Ast.def"

    default:
        UNREACHABLE();
    }
}

// Declarations
FunctionDeclNode *FunctionDeclNode_new(
    DeclaratorNode *declarator, StmtNode *body, Vec(Symbol) * local_variables) {
    assert(declarator);
    assert(body);
    assert(local_variables);

    FunctionDeclNode *p = FunctionDeclNode_alloc();
    p->declarator = declarator;
    p->body = body;
    p->local_variables = local_variables;

    return p;
}

// Misc
TranslationUnitNode *TranslationUnitNode_new(Vec(DeclNode) * declarations) {
    assert(declarations);

    TranslationUnitNode *p = TranslationUnitNode_alloc();
    p->declarations = declarations;

    return p;
}

// Dump
static void Node_dump_impl(const Node *p, FILE *fp, size_t depth);

static void Node_dump_indent(FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < depth; i++) {
        fprintf(fp, "  ");
    }
}

static void Node_dump_symbol(const Symbol *x, FILE *fp, size_t depth) {
    assert(x);
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(symbol %s)\n", x->name);
}

static void Node_dump_int(long long x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(int %lld)\n", x);
}

static void Node_dump_UnaryOp(UnaryOp x, FILE *fp, size_t depth) {
    assert(fp);

    const char *text;
    switch (x) {
#define UNARY_OP(name)                                                         \
    case UnaryOp_##name:                                                       \
        text = #name;                                                          \
        break;
#include "Ast.def"

    default:
        UNREACHABLE();
    }

    Node_dump_indent(fp, depth);
    fprintf(fp, "(UnaryOp %s)\n", text);
}

static void Node_dump_BinaryOp(BinaryOp x, FILE *fp, size_t depth) {
    assert(fp);

    const char *text;
    switch (x) {
#define BINARY_OP(name)                                                        \
    case BinaryOp_##name:                                                      \
        text = #name;                                                          \
        break;
#include "Ast.def"

    default:
        UNREACHABLE();
    }

    Node_dump_indent(fp, depth);
    fprintf(fp, "(BinaryOp %s)\n", text);
}

static void Node_dump_ImplicitCastOp(ImplicitCastOp x, FILE *fp, size_t depth) {
    assert(fp);

    const char *text;
    switch (x) {
    case ImplicitCastOp_lvalue_to_rvalue:
        text = "lvalue_to_rvalue";
        break;

    case ImplicitCastOp_function_to_function_pointer:
        text = "function_to_function_pointer";
        break;

    default:
        UNREACHABLE();
    }

    Node_dump_indent(fp, depth);
    fprintf(fp, "(ImplicitCastOp %s)\n", text);
}

static void Node_dump_Expr(const ExprNode *x, FILE *fp, size_t depth) {
    assert(fp);
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Stmt(const StmtNode *x, FILE *fp, size_t depth) {
    assert(fp);
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Stmts(const Vec(StmtNode) * x, FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < Vec_len(StmtNode)(x); i++) {
        Node_dump_Stmt(Vec_get(StmtNode)(x, i), fp, depth);
    }
}

static void
Node_dump_Declarator(const DeclaratorNode *x, FILE *fp, size_t depth) {
    assert(fp);
    Node_dump_impl((const Node *)x, fp, depth);
}

static void
Node_dump_Declarators(const Vec(DeclaratorNode) * x, FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(x); i++) {
        Node_dump_Declarator(Vec_get(DeclaratorNode)(x, i), fp, depth);
    }
}

static void Node_dump_Decl(const DeclNode *x, FILE *fp, size_t depth) {
    assert(fp);
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Decls(const Vec(DeclNode) * x, FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < Vec_len(DeclNode)(x); i++) {
        Node_dump_Decl(Vec_get(DeclNode)(x, i), fp, depth);
    }
}

static void Node_dump_impl(const Node *p, FILE *fp, size_t depth) {
    assert(fp);

    if (p == NULL) {
        Node_dump_indent(fp, depth);
        fprintf(fp, "(null)\n");
        return;
    }

    // clang-format off
    switch (p->kind) {
#define NODE(name, base)                                                       \
    case NodeKind_##name##base: {                                              \
        const name##base##Node *q = name##base##Node_ccast_node(p);            \
        (void)q;                                                               \
                                                                               \
        Node_dump_indent(fp, depth);                                           \
        fprintf(fp, "(%s\n", #name #base);

#define NODE_MEMBER_F(T, x, f)                                                 \
        Node_dump_##f(q->x, fp, depth + 1);

#define NODE_END()                                                             \
        Node_dump_indent(fp, depth);                                           \
        fprintf(fp, ")\n");                                                    \
        break;                                                                 \
    }
#include "Ast.def"

    default:
        UNREACHABLE();
    }
    // clang-format on
}

void Node_dump(const Node *p, FILE *fp) {
    assert(fp);

    Node_dump_impl(p, fp, 0);
}
