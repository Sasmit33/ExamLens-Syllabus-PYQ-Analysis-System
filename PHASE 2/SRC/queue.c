#include "queue.h"
#include "common.h"

#include <stdlib.h>

void iq_init(IntQueue *q, int capacity)
{
    q->cap = capacity > 4 ? capacity : 4;
    q->a = xmalloc((size_t)q->cap * sizeof *q->a);
    q->head = 0;
    q->size = 0;
}

void iq_push(IntQueue *q, int value)
{
    if (q->size == q->cap) {                  /* grow and unwrap in order */
        int ncap = q->cap * 2;
        int *na = xmalloc((size_t)ncap * sizeof *na);
        for (int i = 0; i < q->size; i++) na[i] = q->a[(q->head + i) % q->cap];
        free(q->a);
        q->a = na;
        q->cap = ncap;
        q->head = 0;
    }
    q->a[(q->head + q->size) % q->cap] = value;
    q->size++;
}

int iq_pop(IntQueue *q)
{
    int v = q->a[q->head];
    q->head = (q->head + 1) % q->cap;
    q->size--;
    return v;
}

int iq_get(const IntQueue *q, int i)
{
    return q->a[(q->head + i) % q->cap];
}

int iq_empty(const IntQueue *q)
{
    return q->size == 0;
}

void iq_clear(IntQueue *q)
{
    q->head = 0;
    q->size = 0;
}

void iq_free(IntQueue *q)
{
    free(q->a);
    q->a = NULL;
    q->size = q->cap = q->head = 0;
}
