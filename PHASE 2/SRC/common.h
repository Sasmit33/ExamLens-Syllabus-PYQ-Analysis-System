#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE   2048   /* longest input line accepted               */
#define MAX_TEXT   1024   /* longest normalized question text          */
#define MAX_NAME   128    /* longest topic name                        */
#define MAX_TOKENS 256    /* most words considered in one question     */

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_LIKE(f, a) __attribute__((format(printf, f, a)))
#else
#define PRINTF_LIKE(f, a)
#endif

/* Grow a dynamic array (ptr, count, capacity) so one more element fits. */
#define GROW(ptr, n, cap)                                                   \
    do {                                                                    \
        if ((n) == (cap)) {                                                 \
            (cap) = (cap) ? (cap) * 2 : 16;                                 \
            (ptr) = xrealloc((ptr), (size_t)(cap) * sizeof *(ptr));         \
        }                                                                   \
    } while (0)

void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

void strip_newline(char *s);
char *trim(char *s);                       /* in place; returns start of trimmed text */
int  str_ieq(const char *a, const char *b);/* case-insensitive equality              */

/* Split on `sep` in place. Keeps EMPTY fields (unlike strtok). The last field
   receives the rest of the line, so question text may contain the separator. */
int split_fields(char *line, char sep, char **fields, int max_fields);

/* Lowercase, punctuation -> space, collapse spaces, drop tokens with no letter
   (pure numbers like "50"). Keeps '+' so "c++" and "b+" survive. */
void normalize_text(const char *in, char *out, size_t out_size);

/* Split a normalized string on spaces, in place. */
int tokenize(char *s, char **tokens, int max_tokens);

#ifdef __cplusplus
}
#endif

#endif
