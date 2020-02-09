#include "mocc.h"

#define NODE(name, base)                                                       \
    Node *name##base##Node_base_node(name##base##Node *p) {                    \
        assert(p);                                                             \
        return (Node *)p;                                                      \
    }                                                                          \
    const Node *name##base##Node_cbase_node(const name##base##Node *p) {       \
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

EnumeratorExprNode *EnumeratorExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    Symbol *symbol,
    int value) {
    assert(symbol);

    EnumeratorExprNode *p =
        EnumeratorExprNode_alloc(result_type, value_category);
    p->symbol = symbol;
    p->value = value;

    return p;
}

IntegerExprNode *IntegerExprNode_new(
    Type *result_type, ValueCategory value_category, int value) {
    IntegerExprNode *p = IntegerExprNode_alloc(result_type, value_category);
    p->value = value;

    return p;
}

StringExprNode *StringExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    const char *string,
    size_t length) {
    assert(string);

    StringExprNode *p = StringExprNode_alloc(result_type, value_category);
    p->value = string;
    p->length = length;

    return p;
}

SubscriptExprNode *SubscriptExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *array,
    ExprNode *index) {
    assert(array);
    assert(index);

    SubscriptExprNode *p = SubscriptExprNode_alloc(result_type, value_category);
    p->array = array;
    p->index = index;

    return p;
}

CallExprNode *CallExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *callee,
    Vec(ExprNode) * arguments) {
    assert(callee);
    assert(arguments);

    CallExprNode *p = CallExprNode_alloc(result_type, value_category);
    p->callee = callee;
    p->arguments = arguments;

    return p;
}

DotExprNode *DotExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *parent,
    Symbol *member_symbol) {
    assert(parent);
    assert(member_symbol);

    DotExprNode *p = DotExprNode_alloc(result_type, value_category);
    p->parent = parent;
    p->member_symbol = member_symbol;

    return p;
}

ArrowExprNode *ArrowExprNode_new(
    Type *result_type,
    ValueCategory value_category,
    ExprNode *parent,
    Symbol *member_symbol) {
    assert(parent);
    assert(member_symbol);

    ArrowExprNode *p = ArrowExprNode_alloc(result_type, value_category);
    p->parent = parent;
    p->member_symbol = member_symbol;

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

IfStmtNode *
IfStmtNode_new(ExprNode *condition, StmtNode *if_true, StmtNode *if_false) {
    assert(condition);
    assert(if_true);

    IfStmtNode *p = IfStmtNode_alloc();
    p->condition = condition;
    p->if_true = if_true;
    p->if_false = if_false;

    return p;
}

WhileStmtNode *WhileStmtNode_new(ExprNode *condition, StmtNode *body) {
    assert(condition);
    assert(body);

    WhileStmtNode *p = WhileStmtNode_alloc();
    p->condition = condition;
    p->body = body;

    return p;
}

ForStmtNode *ForStmtNode_new(
    StmtNode *initializer,
    ExprNode *condition,
    ExprNode *step,
    StmtNode *body) {
    assert(body);

    ForStmtNode *p = ForStmtNode_alloc();
    p->initializer = initializer;
    p->condition = condition;
    p->step = step;
    p->body = body;

    return p;
}

ReturnStmtNode *ReturnStmtNode_new(ExprNode *return_value) {
    ReturnStmtNode *p = ReturnStmtNode_alloc();
    p->return_value = return_value;

    return p;
}

DeclStmtNode *
DeclStmtNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(decl_spec);
    assert(declarators);

    DeclStmtNode *p = DeclStmtNode_alloc();
    p->decl_spec = decl_spec;
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
DirectDeclaratorNode *DirectDeclaratorNode_new(const char *name) {
    assert(name);

    DirectDeclaratorNode *p = DirectDeclaratorNode_alloc();
    p->name = name;
    p->symbol = NULL;

    return p;
}

static Symbol *DirectDeclaratorNode_symbol(const DirectDeclaratorNode *p) {
    assert(p);

    return p->symbol;
}

static Vec(DeclaratorNode) *
    DirectDeclaratorNode_parameters(const DirectDeclaratorNode *p) {
    assert(p);

    (void)p;
    return NULL;
}

ArrayDeclaratorNode *
ArrayDeclaratorNode_new(DeclaratorNode *declarator, ExprNode *array_size) {
    assert(declarator);

    ArrayDeclaratorNode *p = ArrayDeclaratorNode_alloc();
    p->declarator = declarator;
    p->array_size = array_size;

    return p;
}

static Symbol *ArrayDeclaratorNode_symbol(const ArrayDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_symbol(p->declarator);
}

static Vec(DeclaratorNode) *
    ArrayDeclaratorNode_parameters(const ArrayDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_parameters(p->declarator);
}

FunctionDeclaratorNode *FunctionDeclaratorNode_new(
    DeclaratorNode *declarator,
    Vec(DeclaratorNode) * parameters,
    bool is_var_arg) {
    assert(declarator);
    assert(parameters);

    FunctionDeclaratorNode *p = FunctionDeclaratorNode_alloc();
    p->declarator = declarator;
    p->parameters = parameters;
    p->is_var_arg = is_var_arg;

    return p;
}

static Symbol *FunctionDeclaratorNode_symbol(const FunctionDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_symbol(p->declarator);
}

static Vec(DeclaratorNode) *
    FunctionDeclaratorNode_parameters(const FunctionDeclaratorNode *p) {
    assert(p);

    Vec(DeclaratorNode) *parameters = DeclaratorNode_parameters(p->declarator);
    return parameters == NULL ? p->parameters : parameters;
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

static Vec(DeclaratorNode) *
    PointerDeclaratorNode_parameters(const PointerDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_parameters(p->declarator);
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

static Vec(DeclaratorNode) *
    InitDeclaratorNode_parameters(const InitDeclaratorNode *p) {
    assert(p);

    return DeclaratorNode_parameters(p->declarator);
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

Vec(DeclaratorNode) * DeclaratorNode_parameters(const DeclaratorNode *p) {
    assert(p);

    switch (p->kind) {
#define DECLARATOR_NODE(name)                                                  \
    case NodeKind_##name##Declarator:                                          \
        return name##DeclaratorNode_parameters(name##DeclaratorNode_ccast(p));
#include "Ast.def"

    default:
        UNREACHABLE();
    }
}

// Declarations
GlobalDeclNode *
GlobalDeclNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(decl_spec);
    assert(declarators);

    GlobalDeclNode *p = GlobalDeclNode_alloc();
    p->decl_spec = decl_spec;
    p->declarators = declarators;

    return p;
}

FunctionDeclNode *FunctionDeclNode_new(
    DeclSpecNode *decl_spec,
    DeclaratorNode *declarator,
    StmtNode *body,
    Vec(Symbol) * local_variables) {
    assert(decl_spec);
    assert(declarator);
    assert(body);
    assert(local_variables);

    FunctionDeclNode *p = FunctionDeclNode_alloc();
    p->decl_spec = decl_spec;
    p->declarator = declarator;
    p->body = body;
    p->local_variables = local_variables;

    return p;
}

MemberDeclNode *
MemberDeclNode_new(DeclSpecNode *decl_spec, Vec(DeclaratorNode) * declarators) {
    assert(decl_spec);
    assert(declarators);

    MemberDeclNode *p = MemberDeclNode_alloc();
    p->decl_spec = decl_spec;
    p->declarators = declarators;

    return p;
}

EnumeratorDeclNode *EnumeratorDeclNode_new(Symbol *symbol, ExprNode *value) {
    assert(symbol);

    EnumeratorDeclNode *p = EnumeratorDeclNode_alloc();
    p->symbol = symbol;
    p->value = value;

    return p;
}

// Misc
DeclSpecNode *DeclSpecNode_new(StorageClass storage_class, Type *base_type) {
    assert(base_type);

    DeclSpecNode *p = DeclSpecNode_alloc();
    p->storage_class = storage_class;
    p->base_type = base_type;

    return p;
}

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

static void Node_dump_Symbol(const Symbol *x, FILE *fp, size_t depth) {
    assert(x);
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(symbol %s)\n", x->name);
}

static void Node_dump_bool(bool x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(bool %s)\n", x ? "true" : "false");
}

static void Node_dump_int(int x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(int %d)\n", x);
}

static void Node_dump_string(const char *x, FILE *fp, size_t depth) {
    assert(x);
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(string %s)\n", x);
}

static void Node_dump_size_t(size_t x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(size_t %zu)\n", x);
}

static void Node_dump_StorageClass(StorageClass x, FILE *fp, size_t depth) {
    assert(fp);

    const char *text;
    switch (x) {
    case StorageClass_none:
        text = "none";
        break;

    case StorageClass_typedef:
        text = "typedef";
        break;

    case StorageClass_enum:
        text = "enum";
        break;

    default:
        UNREACHABLE();
    }

    Node_dump_indent(fp, depth);
    fprintf(fp, "(StorageClass %s)\n", text);
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

    case ImplicitCastOp_array_to_pointer:
        text = "array_to_pointer";
        break;

    case ImplicitCastOp_integral_cast:
        text = "integral_cast";
        break;

    default:
        UNREACHABLE();
    }

    Node_dump_indent(fp, depth);
    fprintf(fp, "(ImplicitCastOp %s)\n", text);
}

static void Node_dump_Expr(const ExprNode *x, FILE *fp, size_t depth) {
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Exprs(const Vec(ExprNode) * x, FILE *fp, size_t depth) {
    for (size_t i = 0; i < Vec_len(ExprNode)(x); i++) {
        Node_dump_Expr(Vec_get(ExprNode)(x, i), fp, depth);
    }
}

static void Node_dump_Stmt(const StmtNode *x, FILE *fp, size_t depth) {
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Stmts(const Vec(StmtNode) * x, FILE *fp, size_t depth) {
    for (size_t i = 0; i < Vec_len(StmtNode)(x); i++) {
        Node_dump_Stmt(Vec_get(StmtNode)(x, i), fp, depth);
    }
}

static void
Node_dump_Declarator(const DeclaratorNode *x, FILE *fp, size_t depth) {
    Node_dump_impl((const Node *)x, fp, depth);
}

static void
Node_dump_Declarators(const Vec(DeclaratorNode) * x, FILE *fp, size_t depth) {
    for (size_t i = 0; i < Vec_len(DeclaratorNode)(x); i++) {
        Node_dump_Declarator(Vec_get(DeclaratorNode)(x, i), fp, depth);
    }
}

static void Node_dump_Decl(const DeclNode *x, FILE *fp, size_t depth) {
    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_Decls(const Vec(DeclNode) * x, FILE *fp, size_t depth) {
    for (size_t i = 0; i < Vec_len(DeclNode)(x); i++) {
        Node_dump_Decl(Vec_get(DeclNode)(x, i), fp, depth);
    }
}

static void Node_dump_DeclSpec(const DeclSpecNode *x, FILE *fp, size_t depth) {
    Node_dump_impl(DeclSpecNode_cbase_node(x), fp, depth);
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
