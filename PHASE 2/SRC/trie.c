#include "trie.h"
#include "common.h"

#include <stdlib.h>

static int char_index(char c)
{
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= '0' && c <= '9') return 26 + (c - '0');
    if (c == '+') return 36;
    return -1;
}

TrieNode *trie_create(void)
{
    return xcalloc(1, sizeof(TrieNode));
}

int trie_insert(TrieNode *root, const char *word, int topic)
{
    if (!*word) return 0;
    for (const char *p = word; *p; p++)          /* validate first: no half-built paths */
        if (char_index(*p) < 0) return 0;

    TrieNode *n = root;
    for (const char *p = word; *p; p++) {
        int i = char_index(*p);
        if (!n->child[i]) n->child[i] = trie_create();
        n = n->child[i];
    }
    n->is_end = 1;
    for (int k = 0; k < n->ntopics; k++)
        if (n->topics[k] == topic) return 1;     /* already recorded */
    if (n->ntopics < TRIE_MAX_TOPICS) n->topics[n->ntopics++] = topic;
    return 1;
}

const TrieNode *trie_find(const TrieNode *root, const char *word)
{
    const TrieNode *n = root;
    for (const char *p = word; *p && n; p++) {
        int i = char_index(*p);
        if (i < 0) return NULL;
        n = n->child[i];
    }
    return (n && n->is_end) ? n : NULL;          /* "tre" is a prefix, not a word */
}

void trie_free(TrieNode *root)
{
    if (!root) return;
    for (int i = 0; i < TRIE_ALPHA; i++) trie_free(root->child[i]);
    free(root);
}
