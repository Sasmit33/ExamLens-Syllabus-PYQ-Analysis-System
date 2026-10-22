#include "engine.h"

#include <stdlib.h>
#include <string.h>

/*
 * Scoring = how many words of the question are explained by a topic's keywords.
 *  1. Phrases (multi-word keywords) are matched with KMP, LONGEST FIRST. A matched
 *     span is marked covered so "binary search" inside "binary search tree" and
 *     the word "tree" inside it are not counted again. A phrase earns 1 per word.
 *  2. Remaining uncovered words are looked up in the Trie (1 point). A word shared
 *     by k topics gives each 1/k.
 *  3. Words not in the Trie get a fuzzy chance: edit distance <= 1 to a keyword
 *     of length >= 6 with the SAME first letter (catches typos like "travesal"),
 *     worth 0.5. The first-letter rule exists because testing showed
 *     "notations" -> "rotations" (distance 1) giving AVL trees false credit.
 * confidence = best / (best + second best). A tie gives 0.5.
 */

#define WORD_WEIGHT    1.0
#define FUZZY_WEIGHT   0.5
#define FUZZY_MIN_LEN  6
#define MIN_SCORE      1.0
#define MIN_CONFIDENCE 0.70

static void credit_word(const TrieNode *node, double weight, double *scores)
{
    for (int k = 0; k < node->ntopics; k++)
        scores[node->topics[k]] += weight / node->ntopics;
}

static const TrieNode *fuzzy_lookup(const Engine *e, const char *tok)
{
    int len = (int)strlen(tok);
    if (len < FUZZY_MIN_LEN) return NULL;
    for (int i = 0; i < e->nwords; i++) {
        int wl = (int)strlen(e->words[i]);
        if (wl < FUZZY_MIN_LEN || abs(wl - len) > 1 || e->words[i][0] != tok[0])
            continue;                                            /* cheap filters first */
        if (edit_distance_str(tok, e->words[i]) <= 1) return trie_find(e->trie, e->words[i]);
    }
    return NULL;
}

static void score_question(const Engine *e, const Question *q, double *scores)
{
    int n = (int)strlen(q->norm);
    unsigned char *covered = xcalloc((size_t)n + 1, 1);
    int pos[MAX_TOKENS];

    for (int i = 0; i < e->nph; i++) {
        const Phrase *p = &e->ph[i];
        int cnt = kmp_find_all(q->norm, n, p->text, p->m, p->lps, pos, MAX_TOKENS);
        for (int c = 0; c < cnt; c++) {
            int s = pos[c], free_span = 1;
            for (int k = s; k < s + p->m; k++) {
                if (covered[k]) { free_span = 0; break; }
            }
            if (!free_span) continue;
            memset(covered + s, 1, (size_t)p->m);
            scores[p->topic] += WORD_WEIGHT * p->nwords;
        }
    }

    char buf[MAX_TEXT];
    char *tok[MAX_TOKENS];
    memcpy(buf, q->norm, (size_t)n + 1);
    int nt = tokenize(buf, tok, MAX_TOKENS);
    for (int i = 0; i < nt; i++) {
        if (covered[tok[i] - buf]) continue;                     /* already in a phrase */
        const TrieNode *node = trie_find(e->trie, tok[i]);
        if (node)
            credit_word(node, WORD_WEIGHT, scores);
        else if ((node = fuzzy_lookup(e, tok[i])) != NULL)
            credit_word(node, FUZZY_WEIGHT, scores);
    }
    free(covered);
}

void engine_map_topics(Engine *e)
{
    double *scores = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *scores);
    iq_clear(&e->review);

    for (int i = 0; i < e->nq; i++) {
        Question *q = &e->q[i];
        q->topic = -1;
        q->score = q->confidence = 0.0;
        q->needs_review = 0;
        if (q->is_duplicate) continue;

        for (int t = 0; t < e->nt; t++) scores[t] = 0.0;
        score_question(e, q, scores);

        int best = -1;
        double b1 = 0.0, b2 = 0.0;
        for (int t = 0; t < e->nt; t++) {
            if (scores[t] > b1) { b2 = b1; b1 = scores[t]; best = t; }
            else if (scores[t] > b2) { b2 = scores[t]; }
        }
        q->score = b1;
        q->confidence = b1 > 0 ? b1 / (b1 + b2) : 0.0;
        q->topic = b1 > 0 ? best : -1;               /* tentative even if reviewed */
        q->needs_review = b1 < MIN_SCORE || q->confidence < MIN_CONFIDENCE;
        if (q->needs_review) iq_push(&e->review, i);
    }
    free(scores);
}

/* Called by the dashboard when a human resolves a review-queue item. */
void engine_set_topic(Engine *e, int question, int topic)
{
    if (question < 0 || question >= e->nq || topic < -1 || topic >= e->nt) return;
    Question *q = &e->q[question];
    q->topic = topic;
    q->confidence = 1.0;
    q->needs_review = 0;
}
