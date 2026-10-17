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
