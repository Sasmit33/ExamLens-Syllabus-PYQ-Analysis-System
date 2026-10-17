#include "common.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void die_oom(void)
{
    fprintf(stderr, "fatal: out of memory\n");
    exit(EXIT_FAILURE);
}

void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) die_oom();
    return p;
}

void *xcalloc(size_t n, size_t size)
{
    void *p = calloc(n ? n : 1, size ? size : 1);
    if (!p) die_oom();
    return p;
}

void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) die_oom();
    return q;
}

char *xstrdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = xmalloc(n);
    memcpy(d, s, n);
    return d;
}

void strip_newline(char *s)
{
    s[strcspn(s, "\r\n")] = '\0';
}

char *trim(char *s)
{
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

int str_ieq(const char *a, const char *b)
{
    for (; *a && *b; a++, b++)
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
    return *a == *b;
}

int split_fields(char *line, char sep, char **fields, int max_fields)
{
    int n = 0;
    char *start = line;
    for (;;) {
        fields[n++] = start;
        if (n == max_fields) break;          /* last field keeps the remainder */
        char *p = strchr(start, sep);
        if (!p) break;
        *p = '\0';
        start = p + 1;
    }
    return n;
}

void normalize_text(const char *in, char *out, size_t out_size)
{
    char tmp[MAX_LINE];
    size_t j = 0;
    int last_space = 1;

    if (out_size == 0) return;

    /* Pass 1: lowercase; every non [a-z0-9+] character becomes one space. */
    for (size_t i = 0; in[i] && j < sizeof tmp - 1; i++) {
        unsigned char c = (unsigned char)in[i];
        if (isalnum(c) || c == '+') {
            tmp[j++] = (char)tolower(c);
            last_space = 0;
        } else if (!last_space) {
            tmp[j++] = ' ';
            last_space = 1;
        }
    }
    tmp[j] = '\0';

    /* Pass 2: keep only tokens that contain a letter (drops "50", "38", "+"). */
    size_t o = 0;
    char *p = tmp;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        char *start = p;
        int has_alpha = 0;
        while (*p && *p != ' ') {
            if (isalpha((unsigned char)*p)) has_alpha = 1;
            p++;
        }
        size_t len = (size_t)(p - start);
        if (!has_alpha) continue;
        if (o + len + 2 > out_size) break;   /* truncate safely */
        if (o > 0) out[o++] = ' ';
        memcpy(out + o, start, len);
        o += len;
    }
    out[o] = '\0';
}

int tokenize(char *s, char **tokens, int max_tokens)
{
    int n = 0;
    char *p = s;
    while (*p && n < max_tokens) {
        while (*p == ' ') p++;
        if (!*p) break;
        tokens[n++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return n;
}
