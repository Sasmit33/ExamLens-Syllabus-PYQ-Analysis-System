#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>






int engine_apply_overrides(Engine *e, const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[MAX_LINE];
    int ln = 0, applied = 0;
    while (fgets(line, sizeof line, f)) {
        ln++;
        strip_newline(line);
        char *s = trim(line);
        if (!*s || *s == '#') continue;

        char *fld[3];
        if (split_fields(s, '|', fld, 3) < 3) {
            engine_add_issue(e, "overrides line %d: expected 'YEAR | QID | topic'", ln);
            continue;
        }
        int year = atoi(trim(fld[0]));
        char *qid = trim(fld[1]);
        char *name = trim(fld[2]);

        int topic = -1;
        if (strcmp(name, "-") != 0) {
            char key[MAX_TEXT];
            engine_normalize(e, name, key, sizeof key);
            topic = hm_get_int(e->topic_index, key, -1);
            if (topic < 0) {
                engine_add_issue(e, "overrides line %d: unknown topic '%s'", ln, name);
                continue;
            }
        }

        int found = 0;
        for (int i = 0; i < e->nq && !found; i++) {
            const Question *q = &e->q[i];
            if (!q->is_duplicate && q->year == year && str_ieq(q->qid, qid)) {
                engine_set_topic(e, i, topic);
                found = 1;
                applied++;
            }
        }
        if (!found)
            engine_add_issue(e, "overrides line %d: no question %s in %d", ln, qid, year);
    }
    fclose(f);
    return applied;
}
