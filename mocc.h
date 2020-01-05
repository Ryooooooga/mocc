#ifndef INCLUDE_mocc_h
#define INCLUDE_mocc_h

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros
#define UNIMPLEMENTED() assert(!"unimplemented")
#define TODO(s) assert(!"todo: " s)

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

// Token
typedef int TokenKind;

enum {
    TokenKind_identifier = 256,
    TokenKind_number,
};

typedef struct Token {
    TokenKind kind;
    char *text; // For identifer and number
} Token;

// Lexer
typedef struct Lexer Lexer;

Lexer *Lexer_new(const char *filename, const char *text);
Token *Lexer_read(Lexer *l);

// Preprocessor
Vec(Token) * Preprocessor_read(const char *filename, const char *text);

// Tests
#define ERROR(...) (fprintf(stderr, __VA_ARGS__), exit(1))

void test_Vec(void);
void test_Lexer(void);
void test_Preprocessor(void);

#endif // INCLUDE_mocc_h
