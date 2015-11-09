#include <string.h>
#include <stdio.h>

#include "fileio.h"

/* `strnlen` not widely supported. */
static inline size_t pstrnlen(const char *s, size_t max_len)
{
    const char *end = memchr (s, 0, max_len);
    return end ? (size_t)(end - s) : max_len;
}
#undef strnlen
#define strnlen pstrnlen

/* Like strndup except len == -1 maps to strdup. */
char *fb_copy_path(const char *path, int len)
{
    size_t n;
    char *s;

    n = len >= 0 ? strnlen(path, (size_t)len) : strlen(path);
    if ((s = malloc(n + 1))) {
        memcpy(s, path, n);
        s[n] = '\0';
    }
    return s;
}

int fb_chomp(const char *path, int len, const char *ext)
{
    int ext_len = ext ? strlen(ext) : 0;
    if (len > ext_len && 0 == strncmp(path + len - ext_len, ext, ext_len)) {
        len -= ext_len;
    }
    return len;
}

char *fb_create_join_path(const char *prefix, int prefix_len,
        const char *suffix, int suffix_len, const char *ext, int path_sep)
{
    char *path;
    int ext_len = ext ? strlen(ext) : 0;
    int n;

    if (!prefix ||
            (suffix_len > 0 && (suffix[0] == '/' || suffix[0] == '\\')) ||
            (suffix_len > 1 && suffix[1] == ':')) {
        prefix_len = 0;
    }
    if (path_sep && (prefix_len == 0 ||
            (prefix[prefix_len - 1] == '/' || prefix[prefix_len - 1] == '\\'))) {
        path_sep = 0;
    }
    path = malloc(prefix_len + path_sep + suffix_len + ext_len + 1);
    if (!path) {
        return 0;
    }
    n = 0;
    memcpy(path, prefix, prefix_len);
    n += prefix_len;
    if (path_sep) {
        path[n++] = '/';
    }
    memcpy(path + n, suffix, suffix_len);
    n += suffix_len;
    memcpy(path + n, ext, ext_len);
    n += ext_len;
    path[n] = '\0';
    return path;
}

int fb_find_basename(const char *path, int len)
{
    char *p = (char *)path;

    p += len;
    while(p != path) {
        --p;
        if (*p == '/' || *p == '\\') {
            ++p;
            break;
        }
    }
    return p - path;
}

char *fb_create_basename(const char *path, int len, const char *ext)
{
    int pos;
    char *s;

    pos = fb_find_basename(path, len);
    path += pos;
    len -= pos;
    len = fb_chomp(path, len, ext);
    if ((s = malloc(len + 1))) {
        memcpy(s, path, len);
        s[len] = '\0';
    }
    return s;
}

char *fb_read_file(const char *filename, size_t max_size, size_t *size_out)
{
    FILE *fp;
    size_t size, pos, n, _out;
    char *buf;

    size_out = size_out ? size_out : &_out;

    fp = fopen(filename, "rb");
    size = 0;
    buf = 0;

    if (!fp) {
        goto fail;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    *size_out = size;
    if (max_size > 0 && size > max_size) {
        goto fail;
    }
    rewind(fp);
    buf = malloc(size);
    if (!buf) {
        goto fail;
    }
    pos = 0;
    while ((n = fread(buf + pos, 1, size - pos, fp))) {
        pos += n;
    }
    if (pos != size) {
        goto fail;
    }
    fclose(fp);
    *size_out = size;
    return buf;

fail:
    if (fp) {
        fclose(fp);
    }
    if (buf) {
        free(buf);
    }
    *size_out = size;
    return 0;
}
