
#include "engine.h"

#include <stdio.h>
#include <string.h>

static int failures = 0, checks = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        checks++;                                                            \
        if (!(cond)) {                                                       \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);           \
            failures++;                                                      \
        }                                                                    \
    } while (0)

static void test_normalize_and_split(void)
{
    char out[MAX_TEXT];
    normalize_text("Construct a BST for 50, 30, 70!", out, sizeof out);
    CHECK(strcmp(out, "construct a bst for") == 0);      
    normalize_text("C++ and B+ tree, non-linear", out, sizeof out);
    CHECK(strcmp(out, "c++ and b+ tree non linear") == 0);

    char l1[] = "Q4 |   | Write merge sort";
    char *f[3];
    CHECK(split_fields(l1, '|', f, 3) == 3);
    CHECK(strcmp(trim(f[1]), "") == 0);                  
    char l2[] = "Q4||text";
    CHECK(split_fields(l2, '|', f, 3) == 3 && f[1][0] == '\0'); 
    char l3[] = "Q1|5|a | b";
    CHECK(split_fields(l3, '|', f, 3) == 3 && strcmp(f[2], "a | b") == 0);
}

static void test_hashmap(void)
{
    HashMap *m = hm_create(16);
    char key[32];
    for (int i = 0; i < 1000; i++) {                     
        snprintf(key, sizeof key, "key%d", i);
        hm_put_int(m, key, i * 3);
    }
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        snprintf(key, sizeof key, "key%d", i);
        ok &= hm_get_int(m, key, -1) == i * 3;
    }
    CHECK(ok);
    CHECK(m->size == 1000);
    CHECK(hm_get_int(m, "missing", -7) == -7);
    CHECK(hm_put(m, "key5", NULL) == 0);                 
    hm_free(m, NULL);
}

static void test_trie(void)
{
    TrieNode *t = trie_create();
    CHECK(trie_insert(t, "tree", 1));
    CHECK(trie_insert(t, "tree", 2));
    CHECK(trie_insert(t, "trie", 3));
    CHECK(!trie_insert(t, "bad-word", 4));               
    const TrieNode *n = trie_find(t, "tree");
    CHECK(n && n->ntopics == 2);
    CHECK(trie_find(t, "tre") == NULL);                  
    CHECK(trie_find(t, "trees") == NULL);
    trie_free(t);
}

static void test_kmp(void)
{
    int lps[8];
    int e1[] = {0, 0, 1, 2, 0};
    kmp_build_lps("ababc", 5, lps);
    CHECK(memcmp(lps, e1, sizeof e1) == 0);
    int e2[] = {0, 1, 2, 0};
    kmp_build_lps("aaab", 4, lps);
    CHECK(memcmp(lps, e2, sizeof e2) == 0);

    const char *text = "merge sort and quick sort sorted";
    int pos[8];
    kmp_build_lps("sort", 4, lps);
    int c = kmp_find_all(text, (int)strlen(text), "sort", 4, lps, pos, 8);
    CHECK(c == 2 && pos[0] == 6 && pos[1] == 21);        
}

static void test_edit_distance(void)
{
    CHECK(edit_distance_str("kitten", "sitting") == 3);
    CHECK(edit_distance_str("", "abc") == 3);
    CHECK(edit_distance_str("traversal", "traversal") == 0);
    CHECK(edit_distance_str("travesal", "traversal") == 1);
    char *a[] = {"merge", "sort", "algorithm", "trace"};
    char *b[] = {"merge", "sort", "algorithm", "working"};
    CHECK(edit_distance_tokens(a, 4, b, 4) == 1);
    CHECK(edit_distance_tokens(a, 4, b, 0) == 4);
}

static void test_heap(void)
{
    MaxHeap h;
    heap_init(&h, 2);                                    
    double keys[] = {3, 1, 4, 1, 5};
    for (int i = 0; i < 5; i++) heap_push(&h, keys[i], i);
    int expect[] = {4, 2, 0, 1, 3};                      
    int ok = 1;
    for (int i = 0; i < 5; i++) ok &= heap_pop(&h).id == expect[i];
    CHECK(ok && heap_empty(&h));
    heap_free(&h);

    heap_build(&h, keys, 5);
    CHECK(heap_pop(&h).key == 5.0);
    heap_free(&h);
}

static void test_queue(void)
{
    IntQueue q;
    iq_init(&q, 2);
    for (int i = 0; i < 10; i++) iq_push(&q, i);         
    for (int i = 0; i < 5; i++) iq_pop(&q);
    for (int i = 10; i < 20; i++) iq_push(&q, i);
    int ok = 1;
    for (int i = 5; i < 20; i++) ok &= iq_pop(&q) == i;
    CHECK(ok && iq_empty(&q));
    iq_free(&q);
}

static void test_graph(void)
{
    Graph *g = graph_create(3);                          
    graph_add_edge(g, 0, 2);
    graph_add_edge(g, 1, 2);
    CHECK(graph_add_edge(g, 0, 2) == 0);                 
    double prio[] = {0.1, 0.9, 0.5};
    int order[3];
    CHECK(graph_topo_priority(g, prio, order) == 3);
    CHECK(order[0] == 1 && order[1] == 0 && order[2] == 2); 
    int cyc[3];
    CHECK(graph_find_cycle(g, cyc) == 0);

    int level[3];
    Graph *r = graph_reverse(g);
    graph_bfs(r, 2, level);
    CHECK(level[2] == 0 && level[0] == 1 && level[1] == 1);
    graph_free(r);
    graph_free(g);

    g = graph_create(3);                                 
    graph_add_edge(g, 0, 1);
    graph_add_edge(g, 1, 2);
    graph_add_edge(g, 2, 0);
    CHECK(graph_find_cycle(g, cyc) == 3);
    CHECK(graph_topo_priority(g, prio, order) == 0);     
    graph_free(g);
}

int main(void)
{
    test_normalize_and_split();
    test_hashmap();
    test_trie();
    test_kmp();
    test_edit_distance();
    test_heap();
    test_queue();
    test_graph();
    if (failures)
        printf("%d of %d checks FAILED\n", failures, checks);
    else
        printf("all %d checks passed\n", checks);
    return failures ? 1 : 0;
}
