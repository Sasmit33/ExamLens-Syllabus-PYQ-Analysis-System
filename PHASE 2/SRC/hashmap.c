#include "hashmap.h"
#include "common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

unsigned long hm_hash(const char *s)
{
    unsigned long h = 5381;                      /* djb2: h = h * 33 + c */
    int c;
    while ((c = (unsigned char)*s++) != 0)
        h = ((h << 5) + h) + (unsigned long)c;
    return h;
}

HashMap *hm_create(size_t initial_buckets)
{
    HashMap *m = xmalloc(sizeof *m);
    m->nbuckets = initial_buckets < 16 ? 16 : initial_buckets;
    m->buckets = xcalloc(m->nbuckets, sizeof *m->buckets);
    m->size = 0;
    return m;
}

static void hm_resize(HashMap *m)
{
    size_t nb = m->nbuckets * 2;
    HMEntry **fresh = xcalloc(nb, sizeof *fresh);
    for (size_t i = 0; i < m->nbuckets; i++) {
        HMEntry *e = m->buckets[i];
        while (e) {                              /* rehash every entry */
            HMEntry *next = e->next;
            size_t b = hm_hash(e->key) % nb;
            e->next = fresh[b];
            fresh[b] = e;
            e = next;
        }
    }
    free(m->buckets);
    m->buckets = fresh;
    m->nbuckets = nb;
}

int hm_put(HashMap *m, const char *key, void *value)
{
    size_t b = hm_hash(key) % m->nbuckets;
    for (HMEntry *e = m->buckets[b]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            e->value = value;
            return 0;
        }
    }
    if ((double)(m->size + 1) / (double)m->nbuckets > 0.75) {
        hm_resize(m);
        b = hm_hash(key) % m->nbuckets;
    }
    HMEntry *e = xmalloc(sizeof *e);
    e->key = xstrdup(key);
    e->value = value;
    e->next = m->buckets[b];                     /* insert at chain head */
    m->buckets[b] = e;
    m->size++;
    return 1;
}

void *hm_get(const HashMap *m, const char *key, int *found)
{
    size_t b = hm_hash(key) % m->nbuckets;
    for (HMEntry *e = m->buckets[b]; e; e = e->next) {
        if (strcmp(e->key, key) == 0) {
            if (found) *found = 1;
            return e->value;
        }
    }
    if (found) *found = 0;
    return NULL;
}

int hm_contains(const HashMap *m, const char *key)
{
    int found;
    hm_get(m, key, &found);
    return found;
}

void hm_put_int(HashMap *m, const char *key, int value)
{
    hm_put(m, key, (void *)(intptr_t)value);
}

int hm_get_int(const HashMap *m, const char *key, int default_value)
{
    int found;
    void *v = hm_get(m, key, &found);
    return found ? (int)(intptr_t)v : default_value;
}

void hm_free(HashMap *m, void (*free_value)(void *))
{
    if (!m) return;
    for (size_t i = 0; i < m->nbuckets; i++) {
        HMEntry *e = m->buckets[i];
        while (e) {
            HMEntry *next = e->next;
            free(e->key);
            if (free_value) free_value(e->value);
            free(e);
            e = next;
        }
    }
    free(m->buckets);
    free(m);
}
