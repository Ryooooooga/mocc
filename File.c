#include "mocc.h"

char *File_read(const char *path) {
    assert(path);

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long ssize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (ssize < 0) {
        fclose(fp);
        return NULL;
    }

    size_t size = (size_t)ssize;
    char *s = malloc(sizeof(char) * (size + 1));
    if (fread(s, 1, size, fp) != size) {
        fclose(fp);
        return NULL;
    }

    s[size] = '\0';

    fclose(fp);
    return s;
}
