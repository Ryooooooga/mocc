#include "mocc.h"

static void
check_dump(const char *test_name, const Node *p, const char *expected) {
    assert(test_name);
    assert(expected);

    char path[256];
    snprintf(path, 256, "tmp/%s.txt", test_name);

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        ERROR("could not create file %s", path);
    }

    Node_dump(p, fp);
    fclose(fp);

    char *actual = File_read(path);
    if (actual == NULL) {
        ERROR("could not read file %s", path);
    }

    remove(path);

    if (strcmp(actual, expected) != 0) {
        ERROR(
            "%s: actual != expected\n"
            "expected:\n"
            "---\n"
            "%s\n"
            "---\n"
            "actual:\n"
            "---\n"
            "%s\n"
            "---\n",
            test_name,
            expected,
            actual);
    }
}

void test_Ast(void) {
    check_dump("null", NULL, "(null)\n");
}
