#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif




typedef struct HMEntry {
    char *key;
    void *value;
    struct HMEntry *next;
} HMEntry;

typedef struct {
    HMEntry **buckets;
    size_t nbuckets;
    size_t size;
} HashMap;

unsigned long hm_hash(const char *s);
HashMap *hm_create(size_t initial_buckets);
int   hm_put(HashMap *m, const char *key, void *value); 
void *hm_get(const HashMap *m, const char *key, int *found);
int   hm_contains(const HashMap *m, const char *key);


void hm_put_int(HashMap *m, const char *key, int value);
int  hm_get_int(const HashMap *m, const char *key, int default_value);

void hm_free(HashMap *m, void (*free_value)(void *));

#ifdef __cplusplus
}
#endif

#endif
