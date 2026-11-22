#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINE   2048   
#define MAX_TEXT   1024   
#define MAX_NAME   128    
#define MAX_TOKENS 256    

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_LIKE(f, a) __attribute__((format(printf, f, a)))
#else
#define PRINTF_LIKE(f, a)
#endif


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
char *trim(char *s);                       
int  str_ieq(const char *a, const char *b);



int split_fields(char *line, char sep, char **fields, int max_fields);



void normalize_text(const char *in, char *out, size_t out_size);


int tokenize(char *s, char **tokens, int max_tokens);

#ifdef __cplusplus
}
#endif

#endif
