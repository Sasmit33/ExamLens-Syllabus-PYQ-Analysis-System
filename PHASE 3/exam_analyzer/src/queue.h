#ifndef QUEUE_H
#define QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
    int *a;
    int head;
    int size;
    int cap;
} IntQueue;

void iq_init(IntQueue *q, int capacity);
void iq_push(IntQueue *q, int value);
int  iq_pop(IntQueue *q);              
int  iq_get(const IntQueue *q, int i); 
int  iq_empty(const IntQueue *q);
void iq_clear(IntQueue *q);
void iq_free(IntQueue *q);

#ifdef __cplusplus
}
#endif

#endif
