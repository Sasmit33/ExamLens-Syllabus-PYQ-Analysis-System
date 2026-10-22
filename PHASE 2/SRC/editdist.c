#include "editdist.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

static int min3(int a, int b, int c)
{
    int m = a < b ? a : b;
    return m < c ? m : c;
}

int edit_distance_str(const char *a, const char *b)
{
    int n = (int)strlen(a), m = (int)strlen(b);
    int *prev = xmalloc(((size_t)m + 1) * sizeof *prev);
    int *cur  = xmalloc(((size_t)m + 1) * sizeof *cur);

    for (int j = 0; j <= m; j++) prev[j] = j;
    for (int i = 1; i <= n; i++) {
        cur[0] = i;
        for (int j = 1; j <= m; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            cur[j] = min3(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost);
        }
        int *t = prev; prev = cur; cur = t;
    }
    int d = prev[m];
    free(prev);
    free(cur);
    return d;
}

int edit_distance_tokens(char *const *a, int n, char *const *b, int m)
{
    int *prev = xmalloc(((size_t)m + 1) * sizeof *prev);
    int *cur  = xmalloc(((size_t)m + 1) * sizeof *cur);

    for (int j = 0; j <= m; j++) prev[j] = j;
    for (int i = 1; i <= n; i++) {
        cur[0] = i;
        for (int j = 1; j <= m; j++) {
            int cost = strcmp(a[i - 1], b[j - 1]) == 0 ? 0 : 1;
            cur[j] = min3(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost);
        }
        int *t = prev; prev = cur; cur = t;
    }
    int d = prev[m];
    free(prev);
    free(cur);
    return d;
}
