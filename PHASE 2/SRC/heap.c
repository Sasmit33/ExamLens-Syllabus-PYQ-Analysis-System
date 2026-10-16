#include "heap.h"
#include "common.h"

#include <stdlib.h>

static int higher(HeapItem a, HeapItem b)
{
    return a.key > b.key || (a.key == b.key && a.id < b.id);
}

static void swap_items(HeapItem *x, HeapItem *y)
{
    HeapItem t = *x;
    *x = *y;
    *y = t;
}

static void sift_down(MaxHeap *h, int i)
{
    for (;;) {
        int l = 2 * i + 1, r = l + 1, best = i;
        if (l < h->size && higher(h->a[l], h->a[best])) best = l;
        if (r < h->size && higher(h->a[r], h->a[best])) best = r;
        if (best == i) return;
        swap_items(&h->a[i], &h->a[best]);
        i = best;
    }
}

static void sift_up(MaxHeap *h, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (!higher(h->a[i], h->a[parent])) return;
        swap_items(&h->a[i], &h->a[parent]);
        i = parent;
    }
}

void heap_init(MaxHeap *h, int capacity)
{
    h->cap = capacity > 4 ? capacity : 4;
    h->size = 0;
    h->a = xmalloc((size_t)h->cap * sizeof *h->a);
}

void heap_build(MaxHeap *h, const double *keys, int n)
{
    heap_init(h, n);
    for (int i = 0; i < n; i++) {
        h->a[i].key = keys[i];
        h->a[i].id = i;
    }
    h->size = n;
    for (int i = n / 2 - 1; i >= 0; i--) sift_down(h, i);   /* O(n) total */
}

void heap_push(MaxHeap *h, double key, int id)
{
    if (h->size == h->cap) {
        h->cap *= 2;
        h->a = xrealloc(h->a, (size_t)h->cap * sizeof *h->a);
    }
    h->a[h->size].key = key;
    h->a[h->size].id = id;
    sift_up(h, h->size++);
}

HeapItem heap_pop(MaxHeap *h)
{
    HeapItem top = h->a[0];
    h->a[0] = h->a[--h->size];
    sift_down(h, 0);
    return top;
}

int heap_empty(const MaxHeap *h)
{
    return h->size == 0;
}

void heap_free(MaxHeap *h)
{
    free(h->a);
    h->a = NULL;
    h->size = h->cap = 0;
}
