#ifndef HEAP_H
#define HEAP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Binary max-heap of (key, id). Ties broken by smaller id for stable output.
   push/pop O(log n), build from array O(n). */

typedef struct {
    double key;
    int id;
} HeapItem;

typedef struct {
    HeapItem *a;
    int size;
    int cap;
} MaxHeap;

void heap_init(MaxHeap *h, int capacity);
void heap_build(MaxHeap *h, const double *keys, int n);   /* ids 0..n-1, bottom-up heapify */
void heap_push(MaxHeap *h, double key, int id);
HeapItem heap_pop(MaxHeap *h);                            /* caller checks heap_empty first */
int  heap_empty(const MaxHeap *h);
void heap_free(MaxHeap *h);

#ifdef __cplusplus
}
#endif

#endif
