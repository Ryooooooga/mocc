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
}
