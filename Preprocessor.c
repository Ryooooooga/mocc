#include "mocc.h"

static Token *
Token_clone_with_hidden(const Token *t, Vec(String) * hidden_set) {
    assert(t);

    Token *p = malloc(sizeof(Token));
    p->kind = t->kind;
    p->text = t->text;
    p->string = t->string;
    p->string_len = t->string_len;
    p->is_bol = t->is_bol;
    p->has_spaces = t->has_spaces;
    p->hidden_set = Vec_new(String)();

    for (size_t i = 0; i < Vec_len(String)(hidden_set); i++) {
        Vec_push(String)(p->hidden_set, Vec_get(String)(hidden_set, i));
    }

    return p;
}

static bool Token_contains_in_hidden_set(const Token *t, const char *name) {
    assert(t);
    assert(name);

    for (size_t i = 0; i < Vec_len(String)(t->hidden_set); i++) {
        if (strcmp(Vec_get(String)(t->hidden_set, i), name) == 0) {
            return true;
        }
    }

    return false;
}

Macro *
Macro_new(const char *name, Vec(String) * parameters, Vec(Token) * contents) {
    assert(name);
    assert(contents);

    Macro *m = malloc(sizeof(Macro));
    m->name = name;
    m->parameters = parameters;
    m->contents = contents;
    return m;
}

bool Macro_is_function(const Macro *m) {
    assert(m);

    return m->parameters != NULL;
}

typedef struct Preprocessor {
    Vec(String) * include_paths;
    Vec(Macro) * macros;
    Vec(Token) * output;

    Vec(Token) * queue;
    const char *path;
    Lexer *lexer;
} Preprocessor;

static Token *
Preprocessor_read_file(Preprocessor *pp, const char *path, const char *text);

static void Preprocessor_init(Preprocessor *pp, Vec(String) * include_paths) {
    assert(pp);
    assert(include_paths);

    pp->include_paths = include_paths;
    pp->macros = Vec_new(Macro)();
    pp->output = Vec_new(Token)();

    pp->queue = NULL;
    pp->path = NULL;
    pp->lexer = NULL;
}

static const Token *Preprocessor_current(Preprocessor *pp) {
    assert(pp);

    if (Vec_len(Token)(pp->queue) == 0) {
        Vec_push(Token)(pp->queue, Lexer_read(pp->lexer));
    }

    return Vec_get(Token)(pp->queue, 0);
}

static Token *Preprocessor_consume(Preprocessor *pp) {
    assert(pp);

    // Fill lookahead queue with a next token
    Preprocessor_current(pp);

    Token *front = Vec_get(Token)(pp->queue, 0);

    for (size_t i = 1; i < Vec_len(Token)(pp->queue); i++) {
        Token *t = Vec_get(Token)(pp->queue, i);
        Vec_set(Token)(pp->queue, i - 1, t);
    }

    Vec_pop(Token)(pp->queue);
    return front;
}

static void Preprocessor_insert_tokens(
    Preprocessor *pp, const Vec(Token) * tokens, size_t at) {
    assert(pp);
    assert(tokens);
    assert(at <= Vec_len(Token)(pp->queue));

    size_t queue_len = Vec_len(Token)(pp->queue);
    size_t tokens_len = Vec_len(Token)(tokens);

    Vec_resize(Token)(pp->queue, queue_len + tokens_len);

    for (size_t i = at; i < queue_len; i++) {
        Vec_set(Token)(pp->queue, i + tokens_len, Vec_get(Token)(pp->queue, i));
    }

    for (size_t i = 0; i < tokens_len; i++) {
        Vec_set(Token)(pp->queue, i + at, Vec_get(Token)(tokens, i));
    }
}

static const Macro *
Preprocessor_find_macro(const Preprocessor *pp, const char *name) {
    assert(pp);
    assert(name);

    size_t len = Vec_len(Macro)(pp->macros);
    for (size_t i = 0; i < len; i++) {
        const Macro *m = Vec_get(Macro)(pp->macros, i);

        if (strcmp(m->name, name) == 0) {
            return m;
        }
    }

    return NULL;
}

static void Preprocessor_define_macro(Preprocessor *pp, Macro *macro) {
    assert(pp);
    assert(macro);

    if (Preprocessor_find_macro(pp, macro->name) != NULL) {
        ERROR("multiple definition of macro %s\n", macro->name);
    }

    Vec_push(Macro)(pp->macros, macro);
}

static void Preprocessor_parse_define(Preprocessor *pp) {
    assert(pp);
    assert(strcmp(Preprocessor_current(pp)->text, "define") == 0);

    // 'define'
    Preprocessor_consume(pp);

    // identifier
    Token *identifier = Preprocessor_consume(pp);

    if (identifier->is_bol) {
        ERROR("macro name missing\n");
    } else if (identifier->kind != TokenKind_identifier) {
        ERROR("macro name must be an identifier\n");
    }

    Vec(String) *parameters = NULL;

    // ['(']
    const Token *t = Preprocessor_current(pp);
    if (t->kind == '(' && !t->is_bol && !t->has_spaces) {
        // '('
        Preprocessor_consume(pp);

        // parameters
        parameters = Vec_new(String)();

        // [identifier]
        t = Preprocessor_current(pp);
        if (t->kind == TokenKind_identifier && !t->is_bol) {
            UNIMPLEMENTED();
        }

        // ')'
        t = Preprocessor_current(pp);
        if (t->kind != ')' || t->is_bol) {
            ERROR("unexpected end of line, expected ')'\n");
        }

        Preprocessor_consume(pp);
    }

    Vec(Token) *contents = Vec_new(Token)();

    while (!t->is_bol) {
        Vec_push(Token)(contents, Preprocessor_consume(pp));
        t = Preprocessor_current(pp);
    }

    // Register the macro
    Macro *macro = Macro_new(identifier->text, parameters, contents);

    Preprocessor_define_macro(pp, macro);
}

static void Preprocessor_open_file(
    const Preprocessor *pp, const char *hint, char **path, char **text) {
    assert(pp);
    assert(hint);
    assert(path);
    assert(text);

    // TODO: current directory

    for (size_t i = 0; i < Vec_len(String)(pp->include_paths); i++) {
        const char *dir = Vec_get(String)(pp->include_paths, i);
        *path = Path_join(dir, hint);
        *text = File_read(*path);

        fprintf(stderr, "%s\n", *path);

        if (*text != NULL) {
            return;
        }
    }

    ERROR("could not open file %s\n", hint);
}

static void Preprocessor_parse_include(Preprocessor *pp) {
    assert(pp);
    assert(strcmp(Preprocessor_current(pp)->text, "include") == 0);

    // 'include'
    Preprocessor_consume(pp);

    // String literal (TODO: macro, system path)
    const Token *include_file = Preprocessor_consume(pp);

    if (include_file->is_bol) {
        ERROR("missing filename after #include\n");
    } else if (include_file->kind != TokenKind_string) {
        ERROR("expected string literal after #include\n");
    }

    if (!Preprocessor_current(pp)->is_bol) {
        ERROR("extra tokens after #include %s\n", include_file->text);
    }

    const char *hint = include_file->string;
    char *path;
    char *text;

    Preprocessor_open_file(pp, hint, &path, &text);
    Preprocessor_read_file(pp, path, text);
}

static void Preprocessor_parse_directive(Preprocessor *pp) {
    assert(pp);
    assert(Preprocessor_current(pp)->kind == '#');

    // '#'
    Preprocessor_consume(pp);

    const Token *directive = Preprocessor_current(pp);
    if (directive->is_bol) {
        // # <empty>
        return;
    } else if (strcmp(directive->text, "define") == 0) {
        // # define
        Preprocessor_parse_define(pp);
    } else if (strcmp(directive->text, "include") == 0) {
        // # include
        Preprocessor_parse_include(pp);
    } else {
        ERROR("unknown preprocessor directive #%s\n", directive->text);
    }
}

static void Preprocessor_expand_kw(Preprocessor *pp, Token *t) {
    assert(pp);
    assert(t);
    assert(t->kind == TokenKind_identifier);

#define TOKEN_KW(name, text_)                                                  \
    if (strcmp(t->text, text_) == 0) {                                         \
        t->kind = TokenKind_##name;                                            \
        Vec_push(Token)(pp->output, t);                                        \
        return;                                                                \
    }
#include "Token.def"

    Vec_push(Token)(pp->output, t);
}

static void Preprocessor_expand_function_macro(
    Preprocessor *pp, const Macro *m, Token *macro_token) {
    assert(pp);
    assert(m);
    assert(Macro_is_function(m));
    assert(macro_token);

    // '('
    const Token *t = Preprocessor_current(pp);
    if (t->kind != '(') {
        Preprocessor_expand_kw(pp, macro_token);
        return;
    }

    Preprocessor_consume(pp);

    // Macro arguments
    if (Vec_len(String)(m->parameters) != 0) {
        UNIMPLEMENTED();
    }

    // ')'
    t = Preprocessor_current(pp);
    if (t->kind != ')') {
        ERROR("missing ')' near %s\n", t->text);
    }

    Preprocessor_consume(pp);

    // Expand the function macro
    Vec(Token) *tokens = Vec_new(Token)();

    for (size_t i = 0; i < Vec_len(Token)(m->contents); i++) {
        const Token *src_token = Vec_get(Token)(m->contents, i);

        Token *t = Token_clone_with_hidden(src_token, macro_token->hidden_set);
        Vec_push(String)(t->hidden_set, m->name);

        Vec_push(Token)(tokens, t);
    }

    Preprocessor_insert_tokens(pp, tokens, 0);
}

static void Preprocessor_expand_simple_macro(
    Preprocessor *pp, const Macro *m, Token *macro_token) {
    assert(pp);
    assert(m);
    assert(!Macro_is_function(m));
    assert(macro_token);

    Vec(Token) *tokens = Vec_new(Token)();

    for (size_t i = 0; i < Vec_len(Token)(m->contents); i++) {
        const Token *src_token = Vec_get(Token)(m->contents, i);

        Token *t = Token_clone_with_hidden(src_token, macro_token->hidden_set);
        Vec_push(String)(t->hidden_set, m->name);

        Vec_push(Token)(tokens, t);
    }

    Preprocessor_insert_tokens(pp, tokens, 0);
}

static void Preprocessor_expand_identifier(Preprocessor *pp) {
    assert(pp);

    Token *t = Preprocessor_consume(pp);

    assert(t);
    assert(t->kind == TokenKind_identifier);

    const Macro *m = Preprocessor_find_macro(pp, t->text);
    if (m == NULL || Token_contains_in_hidden_set(t, m->name)) {
        // `t` is an identifier or a keyword
        Preprocessor_expand_kw(pp, t);
    } else if (Macro_is_function(m)) {
        // `t` is a function macro
        Preprocessor_expand_function_macro(pp, m, t);
    } else {
        // `t` is a non-function macro
        Preprocessor_expand_simple_macro(pp, m, t);
    }
}

static Token *
Preprocessor_read_file(Preprocessor *pp, const char *path, const char *text) {
    assert(pp);
    assert(path);
    assert(text);

    Vec(Token) *prev_queue = pp->queue;
    Lexer *prev_lexer = pp->lexer;
    const char *prev_path = pp->path;

    pp->queue = Vec_new(Token)();
    pp->lexer = Lexer_new(path, text);
    pp->path = path;

    const Token *t;
    while ((t = Preprocessor_current(pp))->kind != '\0') {
        if (t->kind == '#' && t->is_bol) {
            // Preprocessor directive
            Preprocessor_parse_directive(pp);
        } else if (t->kind == TokenKind_identifier) {
            // Expand identifiers
            Preprocessor_expand_identifier(pp);
        } else {
            // Other tokens
            Token *token = Preprocessor_consume(pp);
            Vec_push(Token)(pp->output, token);
        }
    }

    Token *eof = Preprocessor_consume(pp);

    pp->queue = prev_queue;
    pp->lexer = prev_lexer;
    pp->path = prev_path;

    return eof;
}

Vec(Token) *
    Preprocessor_read(
        Vec(String) * include_paths, const char *path, const char *text) {
    assert(include_paths);
    assert(path);
    assert(text);

    Preprocessor pp;
    Preprocessor_init(&pp, include_paths);

    Token *eof = Preprocessor_read_file(&pp, path, text);
    Vec_push(Token)(pp.output, eof);

    return pp.output;
}
