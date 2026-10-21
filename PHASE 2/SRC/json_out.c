#include "engine.h"

#include <stdio.h>
#include <stdlib.h>

/* Writes every result the dashboard needs as one JSON object.
   All computation (mapping, BFS, ranking, study order) happens in C;
   the web page only displays it. */

static void jstr(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        switch (c) {
        case '"':  fputs("\\\"", f); break;
        case '\\': fputs("\\\\", f); break;
        case '\n': fputs("\\n", f); break;
        case '\r': fputs("\\r", f); break;
        case '\t': fputs("\\t", f); break;
        default:
            if (c < 0x20) fprintf(f, "\\u%04x", c);
            else fputc(c, f);
        }
    }
    fputc('"', f);
}

static void jbool(FILE *f, int v) { fputs(v ? "true" : "false", f); }

/* [[topic, level], ...] for every topic reachable from start, nearest first. */
static void write_levels(FILE *f, const Graph *g, int start, int n)
{
    int *level = xmalloc(((size_t)n + 1) * sizeof *level);
    graph_bfs(g, start, level);
    fputc('[', f);
    int first = 1;
    for (int d = 1; d < n; d++)
        for (int t = 0; t < n; t++)
            if (level[t] == d) {
                fprintf(f, "%s[%d,%d]", first ? "" : ",", t, d);
                first = 0;
            }
    fputc(']', f);
    free(level);
}

void engine_write_json(const Engine *e, FILE *f, double bt, double bb, int k)
{
    int pending = 0;
    for (int i = 0; i < e->review.size; i++)
        pending += e->q[iq_get(&e->review, i)].needs_review;

    fputs("{\"summary\":{", f);
    fprintf(f, "\"questions\":%d,\"topics\":%d,\"min_year\":%d,\"max_year\":%d,", e->nq, e->nt,
            e->min_year, e->max_year);
    fprintf(f, "\"missing_marks\":%d,\"missing_year\":%d,\"duplicates\":%d,\"malformed\":%d,",
            e->n_missing_marks, e->n_missing_year, e->n_duplicates, e->n_malformed);
    fprintf(f, "\"pending_review\":%d},", pending);

    fputs("\"backtest\":{", f);
    if (bt < 0) fputs("\"score\":null,\"baseline\":null", f);
    else fprintf(f, "\"score\":%.4f,\"baseline\":%.4f", bt, bb);
    fprintf(f, ",\"top_k\":%d},", k);

    /* topics, with BFS prerequisites (backwards) and dependents (forwards) */
    Graph *rev = graph_reverse(e->prereq);
    fputs("\"topics\":[", f);
    for (int t = 0; t < e->nt; t++) {
        const Topic *tp = &e->t[t];
        fprintf(f, "%s{\"id\":%d,\"name\":", t ? "," : "", t);
        jstr(f, tp->name);
        fprintf(f, ",\"unit\":%d,\"priority\":%.4f,\"marks\":%.2f,\"count\":%.2f,\"years\":%d,", tp->unit,
                tp->priority, tp->w_marks, tp->w_count, tp->nyears);
        if (tp->last_year < 0) fputs("\"last_year\":null,", f);
        else fprintf(f, "\"last_year\":%d,", tp->last_year);
        fprintf(f, "\"repeats\":%.2f,\"before\":", tp->repeat_score);
        write_levels(f, rev, t, e->nt);
        fputs(",\"after\":", f);
        write_levels(f, e->prereq, t, e->nt);
        fputc('}', f);
    }
    fputs("],", f);
    graph_free(rev);

    int *rank = xmalloc(((size_t)e->nt + 1) * sizeof *rank);
    int nr = engine_rank_topics(e, 0, rank);
    fputs("\"ranking\":[", f);
    for (int i = 0; i < nr; i++) fprintf(f, "%s%d", i ? "," : "", rank[i]);
    free(rank);
    fputs("],\"study_order\":[", f);
    for (int i = 0; i < e->nstudy; i++) fprintf(f, "%s%d", i ? "," : "", e->study_order[i]);
    fputs("],\"study_complete\":", f);
    jbool(f, e->nstudy == e->nt);

    fputs(",\"questions\":[", f);
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        fprintf(f, "%s{\"id\":%d,", i ? "," : "", i);
        if (q->year < 0) fputs("\"year\":null,", f); else fprintf(f, "\"year\":%d,", q->year);
        fputs("\"qid\":", f);
        jstr(f, q->qid);
        if (q->marks < 0) fputs(",\"marks\":null", f); else fprintf(f, ",\"marks\":%d", q->marks);
        if (q->topic < 0) fputs(",\"topic\":null", f); else fprintf(f, ",\"topic\":%d", q->topic);
        fprintf(f, ",\"confidence\":%.4f,\"score\":%.2f,\"needs_review\":", q->confidence, q->score);
        jbool(f, q->needs_review);
        fputs(",\"or_pair\":", f);
        jbool(f, q->in_or_pair);
        fputs(",\"duplicate\":", f);
        jbool(f, q->is_duplicate);
        fprintf(f, ",\"group\":%d,\"text\":", q->group);
        jstr(f, q->raw);
        fputc('}', f);
    }

    fputs("],\"review\":[", f);                 /* FIFO order, unresolved only */
    int first = 1;
    for (int i = 0; i < e->review.size; i++) {
        int qi = iq_get(&e->review, i);
        if (!e->q[qi].needs_review) continue;
        fprintf(f, "%s%d", first ? "" : ",", qi);
        first = 0;
    }

    fputs("],\"repeat_groups\":[", f);
    first = 1;
    for (int g = 0; g < e->ngroups; g++) {
        if (e->group_years[g] < 2) continue;
        fprintf(f, "%s{\"years\":%d,\"questions\":[", first ? "" : ",", e->group_years[g]);
        first = 0;
        int firstq = 1;
        for (int i = 0; i < e->nq; i++)
            if (e->q[i].group == g) {
                fprintf(f, "%s%d", firstq ? "" : ",", i);
                firstq = 0;
            }
        fputs("]}", f);
    }

    fputs("],\"issues\":[", f);
    for (int i = 0; i < e->nissues; i++) {
        if (i) fputc(',', f);
        jstr(f, e->issues[i]);
    }
    fputs("]}\n", f);
}
