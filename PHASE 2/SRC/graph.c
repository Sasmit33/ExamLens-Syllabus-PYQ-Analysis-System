#include "graph.h"
#include "common.h"
#include "heap.h"
#include "queue.h"

#include <stdlib.h>
#include <string.h>

Graph *graph_create(int n)
{
    Graph *g = xmalloc(sizeof *g);
    g->n = n;
    g->adj = xcalloc((size_t)n, sizeof *g->adj);
    g->deg = xcalloc((size_t)n, sizeof *g->deg);
    g->cap = xcalloc((size_t)n, sizeof *g->cap);
    g->indeg = xcalloc((size_t)n, sizeof *g->indeg);
    return g;
}

int graph_add_edge(Graph *g, int u, int v)
{
    if (u < 0 || v < 0 || u >= g->n || v >= g->n) return 0;
    for (int k = 0; k < g->deg[u]; k++)
        if (g->adj[u][k] == v) return 0;              /* ignore duplicate edge */
    if (g->deg[u] == g->cap[u]) {
        g->cap[u] = g->cap[u] ? g->cap[u] * 2 : 4;
        g->adj[u] = xrealloc(g->adj[u], (size_t)g->cap[u] * sizeof **g->adj);
    }
    g->adj[u][g->deg[u]++] = v;
    g->indeg[v]++;
    return 1;
}

Graph *graph_reverse(const Graph *g)
{
    Graph *r = graph_create(g->n);
    for (int u = 0; u < g->n; u++)
        for (int k = 0; k < g->deg[u]; k++) graph_add_edge(r, g->adj[u][k], u);
    return r;
}

/* colour: 0 = unvisited, 1 = on the current DFS path, 2 = finished */
static int dfs_cycle(const Graph *g, int u, int *colour, int *parent, int *cs, int *ce)
{
    colour[u] = 1;
    for (int k = 0; k < g->deg[u]; k++) {
        int v = g->adj[u][k];
        if (colour[v] == 0) {
            parent[v] = u;
            if (dfs_cycle(g, v, colour, parent, cs, ce)) return 1;
        } else if (colour[v] == 1) {                 /* back edge u -> v closes a cycle */
            *cs = v;
            *ce = u;
            return 1;
        }
    }
    colour[u] = 2;
    return 0;
}

int graph_find_cycle(const Graph *g, int *cycle)
{
    int *colour = xcalloc((size_t)g->n, sizeof *colour);
    int *parent = xmalloc(((size_t)g->n + 1) * sizeof *parent);
    int cs = -1, ce = -1, len = 0;

    for (int s = 0; s < g->n && cs < 0; s++) {
        if (colour[s] == 0) {
            parent[s] = -1;
            dfs_cycle(g, s, colour, parent, &cs, &ce);
        }
    }
    if (cs >= 0) {                                   /* walk parents ce -> ... -> cs */
        for (int x = ce; x != cs; x = parent[x]) cycle[len++] = x;
        cycle[len++] = cs;
        for (int i = 0, j = len - 1; i < j; i++, j--) {
            int t = cycle[i]; cycle[i] = cycle[j]; cycle[j] = t;
        }
    }
    free(colour);
    free(parent);
    return len;
}

void graph_bfs(const Graph *g, int src, int *level)
{
    for (int i = 0; i < g->n; i++) level[i] = -1;
    if (src < 0 || src >= g->n) return;

    IntQueue q;
    iq_init(&q, g->n);
    level[src] = 0;
    iq_push(&q, src);
    while (!iq_empty(&q)) {
        int u = iq_pop(&q);
        for (int k = 0; k < g->deg[u]; k++) {
            int v = g->adj[u][k];
            if (level[v] < 0) {
                level[v] = level[u] + 1;
                iq_push(&q, v);
            }
        }
    }
    iq_free(&q);
}

int graph_topo_priority(const Graph *g, const double *priority, int *order)
{
    int *indeg = xmalloc(((size_t)g->n + 1) * sizeof *indeg);
    memcpy(indeg, g->indeg, (size_t)g->n * sizeof *indeg);

    MaxHeap ready;
    heap_init(&ready, g->n);
    for (int i = 0; i < g->n; i++)
        if (indeg[i] == 0) heap_push(&ready, priority[i], i);

    int k = 0;
    while (!heap_empty(&ready)) {
        int u = heap_pop(&ready).id;
        order[k++] = u;
        for (int e = 0; e < g->deg[u]; e++) {
            int v = g->adj[u][e];
            if (--indeg[v] == 0) heap_push(&ready, priority[v], v);   /* all prereqs done */
        }
    }
    heap_free(&ready);
    free(indeg);
    return k;
}

void graph_free(Graph *g)
{
    if (!g) return;
    for (int u = 0; u < g->n; u++) free(g->adj[u]);
    free(g->adj);
    free(g->deg);
    free(g->cap);
    free(g->indeg);
    free(g);
}
