#ifndef TRIE_H
#define TRIE_H

#ifdef __cplusplus
extern "C" {
#endif





#define TRIE_ALPHA      37
#define TRIE_MAX_TOPICS 8

typedef struct TrieNode {
    struct TrieNode *child[TRIE_ALPHA];
    int is_end;
    int ntopics;
    int topics[TRIE_MAX_TOPICS];
} TrieNode;

TrieNode *trie_create(void);
int trie_insert(TrieNode *root, const char *word, int topic); 
const TrieNode *trie_find(const TrieNode *root, const char *word); 
void trie_free(TrieNode *root);

#ifdef __cplusplus
}
#endif

#endif
