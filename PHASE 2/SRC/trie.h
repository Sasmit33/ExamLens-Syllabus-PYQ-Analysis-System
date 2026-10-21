#ifndef TRIE_H
#define TRIE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Trie of single-word syllabus keywords. Alphabet: a-z, 0-9, '+' (37 symbols).
   Each word node stores the topics that use that word, so an ambiguous word
   like "tree" can point to several topics. Lookup is O(L), L = word length. */

#define TRIE_ALPHA      37
#define TRIE_MAX_TOPICS 8

typedef struct TrieNode {
    struct TrieNode *child[TRIE_ALPHA];
    int is_end;
    int ntopics;
    int topics[TRIE_MAX_TOPICS];
} TrieNode;

TrieNode *trie_create(void);
int trie_insert(TrieNode *root, const char *word, int topic); /* 0 if word has unsupported chars */
const TrieNode *trie_find(const TrieNode *root, const char *word); /* NULL unless a full word */
void trie_free(TrieNode *root);

#ifdef __cplusplus
}
#endif

#endif
