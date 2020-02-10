#include "mocc.h"

char *Path_join(const char *dir, const char *rel_path) {
    assert(dir);
    assert(rel_path);

    size_t dir_len = strlen(dir);
    size_t rel_path_len = strlen(rel_path);
    char *path = malloc(sizeof(char) * (dir_len + 1 + rel_path_len + 1));

    if (dir_len == 0) {
        strcpy(path, rel_path);
    } else if (dir[dir_len - 1] == '/') {
        strcpy(path, dir);
        strcat(path, rel_path);
    } else {
        strcpy(path, dir);
        strcat(path, "/");
        strcat(path, rel_path);
    }

    return path;
}

char *Path_dir(const char *path) {
    assert(path);

    size_t len = strlen(path);
    while (len > 0 && path[len - 1] != '/') {
        len = len - 1;
    }

    return strndup(path, len);
}
