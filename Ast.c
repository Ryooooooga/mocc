#include "mocc.h"

#define NODE(name, base)                                                       \
    static name##base##Node *name##base##Node_alloc(void) {                    \
        name##base##Node *p = malloc(sizeof(*p));                              \
        p->kind = NodeKind_##name##base;                                       \
        return p;                                                              \
    }                                                                          \
                                                                               \
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

// Expressions
IdentifierExprNode *IdentifierExprNode_new(Symbol *symbol) {
    assert(symbol);

    IdentifierExprNode *p = IdentifierExprNode_alloc();
    p->symbol = symbol;

    return p;
}

IntegerExprNode *IntegerExprNode_new(long long value) {
    IntegerExprNode *p = IntegerExprNode_alloc();
    p->value = value;

    return p;
}

AssignExprNode *AssignExprNode_new(ExprNode *lhs, ExprNode *rhs) {
    assert(lhs);
    assert(rhs);

    AssignExprNode *p = AssignExprNode_alloc();
    p->lhs = lhs;
    p->rhs = rhs;

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

// Declarations
FunctionDeclNode *FunctionDeclNode_new(const char *name, StmtNode *body) {
    assert(name);
    assert(body);

    FunctionDeclNode *p = FunctionDeclNode_alloc();
    p->name = name;
    p->body = body;

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

static void Node_dump_id(const char *x, FILE *fp, size_t depth) {
    assert(x);
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(id %s)\n", x);
}

static void Node_dump_int(long long x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(int %lld)\n", x);
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
