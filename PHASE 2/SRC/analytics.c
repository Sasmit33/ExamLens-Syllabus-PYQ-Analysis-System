#include "engine.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * priority = W_MARKS    * marks / max_marks          (how much it is worth)
 *          + W_COVERAGE * years_asked / total_years  (how consistently asked)
 *          + W_RECENCY  * recency                    (1 if asked in latest year)
 *          + W_REPEAT   * repeats / max_repeats      (exact/near repeats)
 * Weights are ASSUMPTIONS. Tune them and check with engine_backtest().
 * An OR alternative counts 0.5 because a student answers only one of the pair.
 */

#define W_MARKS    0.35
#define W_COVERAGE 0.25
#define W_RECENCY  0.20
#define W_REPEAT   0.20

static double or_weight(const Question *q)
{
    return q->in_or_pair ? 0.5 : 1.0;
}

static int in_window(const Question *q, int upto_year)
{
    return !q->is_duplicate && q->year >= 0 && q->year <= upto_year;
}

void engine_compute_analytics(Engine *e, int upto_year)
{
    for (int t = 0; t < e->nt; t++) {
        Topic *tp = &e->t[t];
        tp->w_count = tp->w_marks = tp->repeat_score = tp->priority = 0.0;
        tp->nyears = 0;
        tp->last_year = -1;
    }

    int years[MAX_YEARS], ny = 0, first = INT_MAX, last = INT_MIN;
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (!in_window(q, upto_year)) continue;
        int found = 0;
        for (int k = 0; k < ny; k++) found |= years[k] == q->year;
        if (!found && ny < MAX_YEARS) years[ny++] = q->year;
        if (q->year < first) first = q->year;
        if (q->year > last) last = q->year;
    }
    if (ny == 0) return;

    /* Year span of each repeat group, counting ONLY years inside the window.
       Without this, a backtest would "know" that a question repeats in the test year. */
    int *gy = xcalloc((size_t)e->ngroups + 1, sizeof *gy);
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (!in_window(q, upto_year) || q->group < 0) continue;
        int seen = 0;
        for (int j = 0; j < i && !seen; j++)
            seen = in_window(&e->q[j], upto_year) && e->q[j].group == q->group &&
                   e->q[j].year == q->year;
        if (!seen) gy[q->group]++;
    }

    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (!in_window(q, upto_year) || q->topic < 0) continue;
        Topic *tp = &e->t[q->topic];
        double w = or_weight(q);
        tp->w_count += w;
        if (q->marks > 0) tp->w_marks += w * q->marks;
        if (q->group >= 0 && gy[q->group] >= 2) tp->repeat_score += w;

        int found = 0;
        for (int k = 0; k < tp->nyears; k++) found |= tp->years[k] == q->year;
        if (!found && tp->nyears < MAX_YEARS) tp->years[tp->nyears++] = q->year;
        if (q->year > tp->last_year) tp->last_year = q->year;
    }
    free(gy);

    double max_marks = 0.0, max_rep = 0.0;
    for (int t = 0; t < e->nt; t++) {
        if (e->t[t].w_marks > max_marks) max_marks = e->t[t].w_marks;
        if (e->t[t].repeat_score > max_rep) max_rep = e->t[t].repeat_score;
    }
    for (int t = 0; t < e->nt; t++) {
        Topic *tp = &e->t[t];
        double marks_n = max_marks > 0 ? tp->w_marks / max_marks : 0.0;
        double rep_n = max_rep > 0 ? tp->repeat_score / max_rep : 0.0;
        double coverage = (double)tp->nyears / ny;
        double recency = tp->last_year < 0 ? 0.0
                       : 1.0 - (double)(last - tp->last_year) / (last - first + 1);
        tp->priority = W_MARKS * marks_n + W_COVERAGE * coverage +
                       W_RECENCY * recency + W_REPEAT * rep_n;
    }
}

int engine_rank_topics(const Engine *e, int by_marks_only, int *out)
{
    double *key = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *key);
    for (int t = 0; t < e->nt; t++)
        key[t] = by_marks_only ? e->t[t].w_marks : e->t[t].priority;

    MaxHeap h;
    heap_build(&h, key, e->nt);              /* O(n) heapify */
    int k = 0;
    while (!heap_empty(&h)) out[k++] = heap_pop(&h).id;   /* O(log n) each */
    heap_free(&h);
    free(key);
    return k;
}

int engine_study_order(Engine *e)
{
    if (!e->prereq) e->prereq = graph_create(e->nt);

    int *cycle = xmalloc(((size_t)e->nt + 1) * sizeof *cycle);
    int len = graph_find_cycle(e->prereq, cycle);
    if (len > 0) {
        char msg[MAX_LINE];
        size_t o = (size_t)snprintf(msg, sizeof msg, "prerequisite cycle:");
        for (int k = 0; k <= len && o < sizeof msg; k++)
            o += (size_t)snprintf(msg + o, sizeof msg - o, " %s%s",
                                  e->t[cycle[k % len]].name, k < len ? " ->" : "");
        engine_add_issue(e, "%s", msg);
    }
    free(cycle);

    double *prio = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *prio);
    for (int t = 0; t < e->nt; t++) prio[t] = e->t[t].priority;
    free(e->study_order);
    e->study_order = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *e->study_order);
    e->nstudy = graph_topo_priority(e->prereq, prio, e->study_order);
    free(prio);
    return e->nstudy == e->nt;
}

/*
 * Train on years < test_year, then ask: what share of test_year's marks fall in
 * the top-k topics? Baseline = ranking by past marks alone. If the full score
 * does not beat the baseline, the extra signals are not earning their weight.
 * Leaves analytics computed for years < test_year; the caller recomputes.
 */
double engine_backtest(Engine *e, int test_year, int k, double *baseline)
{
    engine_compute_analytics(e, test_year - 1);

    int *rank = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *rank);
    int *rank_b = xmalloc((size_t)(e->nt ? e->nt : 1) * sizeof *rank_b);
    char *top = xcalloc((size_t)e->nt + 1, 1);
    char *top_b = xcalloc((size_t)e->nt + 1, 1);
    engine_rank_topics(e, 0, rank);
    engine_rank_topics(e, 1, rank_b);
    for (int i = 0; i < k && i < e->nt; i++) {
        top[rank[i]] = 1;
        top_b[rank_b[i]] = 1;
    }

    double total = 0.0, hit = 0.0, hit_b = 0.0;
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (q->is_duplicate || q->year != test_year || q->marks <= 0) continue;
        double m = or_weight(q) * q->marks;
        total += m;                               /* unmapped questions count as misses */
        if (q->topic >= 0 && top[q->topic]) hit += m;
        if (q->topic >= 0 && top_b[q->topic]) hit_b += m;
    }
    free(rank);
    free(rank_b);
    free(top);
    free(top_b);

    if (total <= 0) {
        if (baseline) *baseline = -1.0;
        return -1.0;
    }
    if (baseline) *baseline = hit_b / total;
    return hit / total;
}
