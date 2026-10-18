#include "engine.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Ignored when comparing questions for repetition: question verbs and filler
   words differ between years even when the question is the same. */
static const char *STOPWORDS[] = {
    "a", "an", "the", "of", "and", "or", "for", "to", "in", "on", "at", "is",
    "are", "be", "it", "its", "with", "by", "from", "as", "this", "that",
    "these", "what", "how", "why", "which", "write", "explain", "define",
    "give", "show", "describe", "discuss", "state", "using", "any", "all",
    "following", "between", "their", "your", "answer", "example", "examples",
    "suitable", "brief", "short", "note", "notes", "justify", NULL
};

Engine *engine_create(void)
{
    Engine *e = xcalloc(1, sizeof *e);
    e->alias = hm_create(32);
    e->topic_index = hm_create(32);
    e->stopwords = hm_create(64);
    for (int i = 0; STOPWORDS[i]; i++) hm_put_int(e->stopwords, STOPWORDS[i], 1);
    e->trie = trie_create();
    iq_init(&e->review, 16);
    e->min_year = e->max_year = -1;
    return e;
}

void engine_add_issue(Engine *e, const char *fmt, ...)
{
    char buf[MAX_LINE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    GROW(e->issues, e->nissues, e->capissues);
    e->issues[e->nissues++] = xstrdup(buf);
}

/* normalize_text + replace each word that has an alias by its expansion.
   Applied identically to questions, keywords and topic names, so they match. */
void engine_normalize(const Engine *e, const char *raw, char *out, size_t out_size)
{
    char base[MAX_TEXT];
    char *tok[MAX_TOKENS];
    size_t o = 0;

    normalize_text(raw, base, sizeof base);
    int n = tokenize(base, tok, MAX_TOKENS);
    if (out_size == 0) return;
    for (int i = 0; i < n; i++) {
        int found;
        const char *w = hm_get(e->alias, tok[i], &found);
        if (!found) w = tok[i];
        size_t len = strlen(w);
        if (o + len + 2 > out_size) break;
        if (o > 0) out[o++] = ' ';
        memcpy(out + o, w, len);
        o += len;
    }
    out[o] = '\0';
}

void engine_free(Engine *e)
{
    if (!e) return;
    for (int i = 0; i < e->nq; i++) {
        free(e->q[i].raw);
        free(e->q[i].norm);
    }
    free(e->q);
    free(e->t);
    for (int i = 0; i < e->nph; i++) {
        free(e->ph[i].text);
        free(e->ph[i].lps);
    }
    free(e->ph);
    for (int i = 0; i < e->nwords; i++) free(e->words[i]);
    free(e->words);
    hm_free(e->alias, free);
    hm_free(e->topic_index, NULL);
    hm_free(e->stopwords, NULL);
    trie_free(e->trie);
    graph_free(e->prereq);
    iq_free(&e->review);
    free(e->group_size);
    free(e->group_years);
    free(e->study_order);
    for (int i = 0; i < e->nissues; i++) free(e->issues[i]);
    free(e->issues);
    free(e);
}
