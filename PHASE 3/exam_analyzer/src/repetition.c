#include "engine.h"

#include <stdlib.h>
#include <string.h>










#define SIM_THRESHOLD 0.60

static int uf_find(int *parent, int x)
{
    while (parent[x] != x) {
        parent[x] = parent[parent[x]];          
        x = parent[x];
    }
    return x;
}

static void uf_union(int *parent, int a, int b)
{
    a = uf_find(parent, a);
    b = uf_find(parent, b);
    if (a == b) return;
    if (a < b) parent[b] = a; else parent[a] = b; 
}

void engine_detect_repetition(Engine *e)
{
    int n = e->nq;
    char **buf = xcalloc((size_t)n + 1, sizeof *buf);
    char ***tok = xcalloc((size_t)n + 1, sizeof *tok);
    int *ntok = xcalloc((size_t)n + 1, sizeof *ntok);
    int *parent = xmalloc(((size_t)n + 1) * sizeof *parent);

    for (int i = 0; i < n; i++) {
        const Question *q = &e->q[i];
        parent[i] = i;
        if (q->is_duplicate || q->year < 0) continue;
        buf[i] = xstrdup(q->norm);
        char *all[MAX_TOKENS];
        int k = tokenize(buf[i], all, MAX_TOKENS);
        tok[i] = xmalloc(((size_t)k + 1) * sizeof **tok);
        for (int j = 0; j < k; j++)
            if (!hm_contains(e->stopwords, all[j])) tok[i][ntok[i]++] = all[j];
    }

    for (int i = 0; i < n; i++) {
        if (ntok[i] == 0) continue;
        for (int j = i + 1; j < n; j++) {
            if (ntok[j] == 0 || e->q[i].year == e->q[j].year) continue;
            int a = ntok[i], b = ntok[j];
            int longer = a > b ? a : b, shorter = a < b ? a : b;
            if ((double)shorter / longer < SIM_THRESHOLD) continue;
            int d = edit_distance_tokens(tok[i], a, tok[j], b);
            if (1.0 - (double)d / longer >= SIM_THRESHOLD) uf_union(parent, i, j);
        }
    }

    
    int *gid = xmalloc(((size_t)n + 1) * sizeof *gid);
    for (int i = 0; i < n; i++) gid[i] = -1;
    e->ngroups = 0;
    for (int i = 0; i < n; i++) {
        if (ntok[i] == 0) { e->q[i].group = -1; continue; }
        int r = uf_find(parent, i);
        if (gid[r] < 0) gid[r] = e->ngroups++;
        e->q[i].group = gid[r];
    }

    free(e->group_size);
    free(e->group_years);
    e->group_size = xcalloc((size_t)e->ngroups + 1, sizeof *e->group_size);
    e->group_years = xcalloc((size_t)e->ngroups + 1, sizeof *e->group_years);
    for (int i = 0; i < n; i++) {
        int g = e->q[i].group;
        if (g < 0) continue;
        e->group_size[g]++;
        int seen = 0;
        for (int j = 0; j < i && !seen; j++)
            seen = e->q[j].group == g && e->q[j].year == e->q[i].year;
        if (!seen) e->group_years[g]++;
    }

    for (int i = 0; i < n; i++) {
        free(buf[i]);
        free(tok[i]);
    }
    free(buf);
    free(tok);
    free(ntok);
    free(parent);
    free(gid);
}
