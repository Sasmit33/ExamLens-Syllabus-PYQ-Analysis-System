#ifndef GRAPH_H
#define GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Directed graph as adjacency lists. Edge u -> v means "study u before v". */

typedef struct {
    int n;
    int **adj;     /* adj[u] = array of neighbours */
    int *deg;      /* out-degree of u              */
    int *cap;      /* capacity of adj[u]           */
    int *indeg;    /* in-degree of u               */
} Graph;

Graph *graph_create(int n);
int    graph_add_edge(Graph *g, int u, int v);   /* 1 added, 0 duplicate/invalid */
Graph *graph_reverse(const Graph *g);            /* v -> u for every u -> v      */

/* DFS with white/grey/black colouring. If a cycle exists, writes it into
   `cycle` (buffer of size >= n) and returns its length; else returns 0. */
int graph_find_cycle(const Graph *g, int *cycle);

/* BFS from src. level[v] = number of edges from src, or -1 if unreachable. */
void graph_bfs(const Graph *g, int src, int *level);

/* Kahn's topological sort using a MAX-HEAP instead of a plain queue: among all
   topics whose prerequisites are done, the highest-priority one comes next.
   Returns how many nodes were ordered (< n means a cycle blocked the rest). */
int graph_topo_priority(const Graph *g, const double *priority, int *order);

void graph_free(Graph *g);

#ifdef __cplusplus
}
#endif

#endif
