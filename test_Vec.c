#include "mocc.h"

VEC_DECL(Test, int)

void test_Vec(void) {
    Vec(Test) *v = Vec_new(Test)();
    assert(v != NULL);

    assert(Vec_len(Test)(v) == 0);

    Vec_push(Test)(v, 1);
    Vec_push(Test)(v, 2);
    Vec_push(Test)(v, 3);

    assert(Vec_len(Test)(v) == 3);
    assert(Vec_get(Test)(v, 0) == 1);
    assert(Vec_get(Test)(v, 1) == 2);
    assert(Vec_get(Test)(v, 2) == 3);

    Vec_set(Test)(v, 1, -1);

    assert(Vec_len(Test)(v) == 3);
    assert(Vec_get(Test)(v, 0) == 1);
    assert(Vec_get(Test)(v, 1) == -1);
    assert(Vec_get(Test)(v, 2) == 3);

    int x2 = Vec_pop(Test)(v);
    int x1 = Vec_pop(Test)(v);
    int x0 = Vec_pop(Test)(v);

    assert(Vec_len(Test)(v) == 0);
    assert(x0 == 1);
    assert(x1 == -1);
    assert(x2 == 3);

    for (int i = 0; i < 200; i++) {
        Vec_push(Test)(v, i);
    }

    assert(Vec_len(Test)(v) == 200);

    for (int i = 0; i < 200; i++) {
        assert(Vec_get(Test)(v, i) == i);
    }
}

VEC_DEFINE(Test)
