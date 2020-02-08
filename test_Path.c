#include "mocc.h"

void test_Path(void) {
    assert(strcmp(Path_join("", "test.c"), "test.c") == 0);
    assert(strcmp(Path_join("", "te/st.c"), "te/st.c") == 0);
    assert(strcmp(Path_join(".", "test.c"), "./test.c") == 0);
    assert(strcmp(Path_join(".", "te/st.c"), "./te/st.c") == 0);
    assert(strcmp(Path_join("./", "test.c"), "./test.c") == 0);
    assert(strcmp(Path_join("./", "te/st.c"), "./te/st.c") == 0);
    assert(strcmp(Path_join("a", "test.c"), "a/test.c") == 0);
    assert(strcmp(Path_join("a", "te/st.c"), "a/te/st.c") == 0);
    assert(strcmp(Path_join("a/", "test.c"), "a/test.c") == 0);
    assert(strcmp(Path_join("a/", "te/st.c"), "a/te/st.c") == 0);
    assert(strcmp(Path_join("/", "test.c"), "/test.c") == 0);
    assert(strcmp(Path_join("/", "te/st.c"), "/te/st.c") == 0);
    assert(strcmp(Path_join("/a/b", "test.c"), "/a/b/test.c") == 0);
    assert(strcmp(Path_join("/a/b", "te/st.c"), "/a/b/te/st.c") == 0);

    assert(strcmp(Path_dir(""), "") == 0);
    assert(strcmp(Path_dir("test.c"), "") == 0);
    assert(strcmp(Path_dir("./"), "./") == 0);
    assert(strcmp(Path_dir("./test.c"), "./") == 0);
    assert(strcmp(Path_dir("a/b"), "a/") == 0);
    assert(strcmp(Path_dir("a/b/"), "a/b/") == 0);
    assert(strcmp(Path_dir("a/b/c"), "a/b/") == 0);
    assert(strcmp(Path_dir("a/"), "a/") == 0);
    assert(strcmp(Path_dir("/"), "/") == 0);
    assert(strcmp(Path_dir("/a/b"), "/a/") == 0);
    assert(strcmp(Path_dir("/a/b/"), "/a/b/") == 0);
}
