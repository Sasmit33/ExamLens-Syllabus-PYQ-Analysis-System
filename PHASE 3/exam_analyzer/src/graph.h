#ifndef GRAPH_H
#define GRAPH_H

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
    int n;
    int **adj;     
    int *deg;      
    int *cap;      
    int *indeg;    
} Graph;

Graph *graph_create(int n);
int    graph_add_edge(Graph *g, int u, int v);   
Graph *graph_reverse(const Graph *g);            



int graph_find_cycle(const Graph *g, int *cycle);


void graph_bfs(const Graph *g, int src, int *level);




int graph_topo_priority(const Graph *g, const double *priority, int *order);

void graph_free(Graph *g);

#ifdef __cplusplus
}
#endif

#endif
