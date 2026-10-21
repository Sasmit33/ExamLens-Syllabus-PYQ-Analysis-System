#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BACKTEST_TOP_K 3

static void section(const char *title)
{
    printf("\n=== %s ===\n", title);
}

static const char *topic_name(const Engine *e, int t)
{
    return t >= 0 ? e->t[t].name : "(unmapped)";
}

static void print_mapping(const Engine *e)
{
    section("Question -> topic mapping");
    printf("%-5s %-6s %-26s %5s %5s  %s\n", "Year", "QID", "Topic", "Score", "Conf", "Flags");
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (q->is_duplicate) continue;
        printf("%-5d %-6s %-26.26s %5.1f %5.2f  %s%s%s\n", q->year, q->qid,
               topic_name(e, q->topic), q->score, q->confidence,
               q->needs_review ? "REVIEW " : "", q->in_or_pair ? "or-pair " : "",
               q->marks < 0 ? "no-marks" : "");
    }

    section("Review queue (FIFO, low confidence)");
    if (iq_empty(&e->review)) printf("empty (all reviewed, or nothing flagged)\n");
    for (int i = 0; i < e->review.size; i++) {
        const Question *q = &e->q[iq_get(&e->review, i)];
        printf("%2d. [%d %s] best guess: %s (conf %.2f)\n    %.90s\n", i + 1, q->year, q->qid,
               topic_name(e, q->topic), q->confidence, q->raw);
    }
}

/* Pops each flagged question from the review queue (FIFO) and asks the user
   for the correct topic. Runs before analytics, so decisions change the output. */
static void interactive_review(Engine *e)
{
    section("Manual review");
    if (iq_empty(&e->review)) printf("nothing to review\n");
    while (!iq_empty(&e->review)) {
        int qi = iq_pop(&e->review);
        Question *q = &e->q[qi];
        printf("\n[%d %s] %s\n", q->year, q->qid, q->raw);
        printf("Best guess: %s (confidence %.2f)\n", topic_name(e, q->topic), q->confidence);
        for (int t = 0; t < e->nt; t++) printf("  %2d. %s\n", t + 1, e->t[t].name);
        printf("Topic number (Enter = accept guess, 0 = no topic): ");
        fflush(stdout);

        char line[32];
        if (!fgets(line, sizeof line, stdin)) break;    /* input ended: stop reviewing */
        int choice = atoi(line);
        if (line[0] == '\n' || line[0] == '\r')
            engine_set_topic(e, qi, q->topic);           /* accept best guess */
        else if (choice >= 1 && choice <= e->nt)
            engine_set_topic(e, qi, choice - 1);
        else
            engine_set_topic(e, qi, -1);                 /* exclude from analytics */
        printf("-> %s\n", topic_name(e, q->topic));
    }
}

static void print_repetition(const Engine *e)
{
    section("Repeated questions (asked in 2+ years)");
    int shown = 0;
    for (int g = 0; g < e->ngroups; g++) {
        if (e->group_years[g] < 2) continue;
        printf("Group %d - %d years:\n", ++shown, e->group_years[g]);
        for (int i = 0; i < e->nq; i++)
            if (e->q[i].group == g)
                printf("   %d %-6s %.80s\n", e->q[i].year, e->q[i].qid, e->q[i].raw);
    }
    if (!shown) printf("none found\n");
}

static void print_ranking(const Engine *e, const int *rank, int n)
{
    section("Topic priority ranking (max-heap)");
    printf("%-4s %-30s %8s %7s %5s %5s\n", "Rank", "Topic", "Priority", "Marks", "Years", "Last");
    for (int i = 0; i < n; i++) {
        const Topic *t = &e->t[rank[i]];
        char last[12] = "-";
        if (t->last_year >= 0) snprintf(last, sizeof last, "%d", t->last_year);
        printf("%-4d %-30.30s %8.3f %7.1f %5d %5s\n", i + 1, t->name, t->priority, t->w_marks,
               t->nyears, last);
    }
}

static void print_study_plan(const Engine *e, const Graph *rev, int top_topic)
{
    section("Study order (topological sort with priority heap)");
    for (int i = 0; i < e->nstudy; i++) {
        int t = e->study_order[i];
        printf("%2d. %-30s priority %.3f", i + 1, e->t[t].name, e->t[t].priority);
        if (rev->deg[t] > 0) {
            printf("   after:");
            for (int k = 0; k < rev->deg[t]; k++)
                printf(" %s%s", e->t[rev->adj[t][k]].name, k + 1 < rev->deg[t] ? "," : "");
        }
        printf("\n");
    }
    if (e->nstudy < e->nt)
        printf("Only %d of %d topics could be ordered: the prerequisites contain a cycle.\n",
               e->nstudy, e->nt);

    if (top_topic < 0) return;
    section("BFS: prerequisites of the highest-priority topic that has any");
    int *level = xmalloc((size_t)e->nt * sizeof *level);
    graph_bfs(rev, top_topic, level);
    printf("Before \"%s\" you need:\n", e->t[top_topic].name);
    int any = 0;
    for (int d = 1; d < e->nt; d++) {
        int printed = 0;
        for (int t = 0; t < e->nt; t++) {
            if (level[t] != d) continue;
            if (!printed) printf("  %d step(s) back:", d);
            printf(" %s;", e->t[t].name);
            printed = any = 1;
        }
        if (printed) printf("\n");
    }
    if (!any) printf("  nothing - no prerequisites\n");
    free(level);
}

int main(int argc, char **argv)
{
    int review = 0, json = 0, nargs = 0;
    const char *overrides = NULL;
    const char *args[3] = {NULL, NULL, NULL};
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--review") == 0) review = 1;
        else if (strcmp(argv[i], "--json") == 0) json = 1;
        else if (strcmp(argv[i], "--overrides") == 0 && i + 1 < argc) overrides = argv[++i];
        else if (nargs < 3) args[nargs++] = argv[i];
    }
    if (nargs < 2) {
        fprintf(stderr, "usage: %s <questions.txt> <syllabus.txt> [prereqs.txt] "
                        "[--review] [--json] [--overrides file]\n", argv[0]);
        return 2;
    }

    Engine *e = engine_create();
    int ok = engine_load_syllabus(e, args[1]) && engine_load_questions(e, args[0]);
    if (ok && nargs > 2) ok = engine_load_prereqs(e, args[2]);
    if (!ok) {
        if (json) {
            printf("{\"error\":\"could not load input files\",\"issues\":[");
            for (int i = 0; i < e->nissues; i++) {
                printf("%s\"", i ? "," : "");
                for (const char *c = e->issues[i]; *c; c++) {
                    if (*c == '"' || *c == '\\') putchar('\\');
                    putchar(*c);
                }
                putchar('"');
            }
            printf("]}\n");
        }
        for (int i = 0; i < e->nissues; i++) fprintf(stderr, "error: %s\n", e->issues[i]);
        engine_free(e);
        return 1;
    }

    engine_validate(e);
    engine_map_topics(e);
    if (overrides) engine_apply_overrides(e, overrides);
    if (review && !json) interactive_review(e);
    engine_detect_repetition(e);

    double bt = -1.0, bt_base = -1.0;
    if (e->max_year > e->min_year) bt = engine_backtest(e, e->max_year, BACKTEST_TOP_K, &bt_base);

    engine_compute_analytics(e, e->max_year);
    engine_study_order(e);

    if (json) {
        engine_write_json(e, stdout, bt, bt_base, BACKTEST_TOP_K);
        engine_free(e);
        return 0;
    }

    section("Summary");
    printf("Questions : %d loaded, years %d-%d\n", e->nq, e->min_year, e->max_year);
    printf("Syllabus  : %d topics, %d keyword phrases (KMP), %d single keywords (Trie)\n",
           e->nt, e->nph, e->nwords);
    printf("Data      : %d missing marks, %d missing year, %d duplicates, %d malformed lines\n",
           e->n_missing_marks, e->n_missing_year, e->n_duplicates, e->n_malformed);

    section("Validation and load issues");
    if (!e->nissues) printf("none\n");
    for (int i = 0; i < e->nissues; i++) printf("- %s\n", e->issues[i]);

    print_mapping(e);
    print_repetition(e);

    int *rank = xmalloc((size_t)e->nt * sizeof *rank);
    int nrank = engine_rank_topics(e, 0, rank);
    print_ranking(e, rank, nrank);

    Graph *rev = graph_reverse(e->prereq);
    int bfs_topic = -1;                       /* highest-ranked topic that HAS prerequisites */
    for (int i = 0; i < nrank && bfs_topic < 0; i++)
        if (rev->deg[rank[i]] > 0) bfs_topic = rank[i];
    print_study_plan(e, rev, bfs_topic);
    graph_free(rev);

    section("Backtest");
    if (bt < 0) {
        printf("needs questions with marks from at least two different years\n");
    } else {
        printf("Trained on %d-%d, tested on %d.\n", e->min_year, e->max_year - 1, e->max_year);
        printf("Top-%d topics by priority cover %.0f%% of %d's marks;\n", BACKTEST_TOP_K,
               100.0 * bt, e->max_year);
        printf("top-%d by past marks alone (baseline) cover %.0f%%.\n", BACKTEST_TOP_K,
               100.0 * bt_base);
        printf("With few years of data this is a sanity check, not proof.\n");
    }

    free(rank);
    engine_free(e);
    return 0;
}
