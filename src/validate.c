#include "engine.h"

#include <stdio.h>

/*
 * Checks:
 *  - missing year (no "YEAR:" line above the question)
 *  - missing marks (kept, but excluded from marks totals)
 *  - exact duplicate text in the same year (data-entry error: ignored)
 *  - same question id twice in one year
 *  - "-or" alternative without its base question
 * Duplicates WITHIN a year are errors; the same question ACROSS years is a
 * signal and is handled by repetition detection, not here.
 */
void engine_validate(Engine *e)
{
    HashMap *seen_text = hm_create(64);
    HashMap *seen_id = hm_create(64);
    HashMap *base_of = hm_create(64);
    char key[MAX_TEXT + 64];

    e->n_missing_year = e->n_missing_marks = e->n_duplicates = 0;

    for (int i = 0; i < e->nq; i++) {
        Question *q = &e->q[i];
        q->is_duplicate = 0;
        q->in_or_pair = 0;

        if (q->year < 0) {
            e->n_missing_year++;
            engine_add_issue(e, "line %d (%s): missing year - add a 'YEAR:' line above it",
                             q->line_no, q->qid);
        }
        if (q->marks < 0) {
            e->n_missing_marks++;
            engine_add_issue(e, "line %d (%s): missing marks - excluded from marks totals",
                             q->line_no, q->qid);
        }

        snprintf(key, sizeof key, "%d|%s", q->year, q->norm);
        int prev = hm_get_int(seen_text, key, -1);
        if (prev >= 0) {
            q->is_duplicate = 1;
            e->n_duplicates++;
            engine_add_issue(e, "line %d (%s): duplicate of line %d - ignored",
                             q->line_no, q->qid, e->q[prev].line_no);
            continue;
        }
        hm_put_int(seen_text, key, i);

        if (q->qnum < 0 || q->year < 0) continue;   /* id checks need a valid year */
        snprintf(key, sizeof key, "%d|%d|%c|%d", q->year, q->qnum, q->sub ? q->sub : '-', q->is_or);
        if (hm_contains(seen_id, key))
            engine_add_issue(e, "line %d: question id %s appears twice in %d",
                             q->line_no, q->qid, q->year);
        else
            hm_put_int(seen_id, key, i);

        if (!q->is_or) {
            snprintf(key, sizeof key, "%d|%d|%c", q->year, q->qnum, q->sub ? q->sub : '-');
            hm_put_int(base_of, key, i);
        }
    }

    /* Link each "-or" alternative to its base: a student answers only one. */
    for (int i = 0; i < e->nq; i++) {
        Question *q = &e->q[i];
        if (!q->is_or || q->is_duplicate || q->qnum < 0) continue;
        snprintf(key, sizeof key, "%d|%d|%c", q->year, q->qnum, q->sub ? q->sub : '-');
        int b = hm_get_int(base_of, key, -1);
        if (b < 0) {
            engine_add_issue(e, "line %d (%s): OR alternative has no matching base question",
                             q->line_no, q->qid);
        } else {
            q->in_or_pair = 1;
            e->q[b].in_or_pair = 1;
        }
    }

    e->min_year = e->max_year = -1;
    for (int i = 0; i < e->nq; i++) {
        const Question *q = &e->q[i];
        if (q->is_duplicate || q->year < 0) continue;
        if (e->min_year < 0 || q->year < e->min_year) e->min_year = q->year;
        if (q->year > e->max_year) e->max_year = q->year;
    }

    hm_free(seen_text, NULL);
    hm_free(seen_id, NULL);
    hm_free(base_of, NULL);
}
