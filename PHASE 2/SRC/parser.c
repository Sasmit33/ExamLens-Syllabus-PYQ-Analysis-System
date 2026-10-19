#include "engine.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- helpers ---------- */

static int starts_with_ci(const char *s, const char *prefix)
{
    for (; *prefix; s++, prefix++)
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
    return 1;
}

/* Reads every line into memory so line numbers stay correct in messages. */
static int read_lines(Engine *e, const char *path, char ***out, int *count)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        engine_add_issue(e, "cannot open file '%s'", path);
        return 0;
    }
    char **lines = NULL;
    int n = 0, cap = 0;
    char buf[MAX_LINE];
    while (fgets(buf, sizeof buf, f)) {
        if (!strchr(buf, '\n') && !feof(f)) {           /* line too long */
            int c;
            while ((c = fgetc(f)) != '\n' && c != EOF) {}
            engine_add_issue(e, "%s line %d: longer than %d characters, skipped",
                             path, n + 1, MAX_LINE - 2);
            buf[0] = '\0';
        }
        strip_newline(buf);
        char *s = buf;
        if (n == 0 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB &&
            (unsigned char)s[2] == 0xBF)
            s += 3;                                     /* UTF-8 BOM from Notepad */
        GROW(lines, n, cap);
        lines[n++] = xstrdup(s);
    }
    fclose(f);
    *out = lines;
    *count = n;
    return 1;
}

static void free_lines(char **lines, int n)
{
    for (int i = 0; i < n; i++) free(lines[i]);
    free(lines);
}

/* ---------- syllabus ---------- */

static int add_topic(Engine *e, const char *name, int unit)
{
    char key[MAX_TEXT];
    engine_normalize(e, name, key, sizeof key);
    if (!*key) return -1;
    if (hm_contains(e->topic_index, key)) return -2;

    GROW(e->t, e->nt, e->capt);
    Topic *t = &e->t[e->nt];
    memset(t, 0, sizeof *t);
    snprintf(t->name, sizeof t->name, "%s", name);
    t->unit = unit;
    t->last_year = -1;
    hm_put_int(e->topic_index, key, e->nt);
    return e->nt++;
}

static void add_keyword(Engine *e, int topic, const char *keyword)
{
    char norm[MAX_TEXT];
    engine_normalize(e, keyword, norm, sizeof norm);
    if (!*norm) return;

    if (!strchr(norm, ' ')) {                           /* single word -> Trie */
        int existed = trie_find(e->trie, norm) != NULL;
        if (!trie_insert(e->trie, norm, topic)) {
            engine_add_issue(e, "keyword '%s' has unsupported characters", keyword);
            return;
        }
        if (!existed) {
            GROW(e->words, e->nwords, e->capwords);
            e->words[e->nwords++] = xstrdup(norm);
        }
        return;
    }

    for (int i = 0; i < e->nph; i++)                    /* phrase -> KMP list */
        if (e->ph[i].topic == topic && strcmp(e->ph[i].text, norm) == 0) return;
    GROW(e->ph, e->nph, e->capph);
    Phrase *p = &e->ph[e->nph++];
    p->text = xstrdup(norm);
    p->m = (int)strlen(norm);
    p->topic = topic;
    p->nwords = 1;
    for (const char *c = norm; *c; c++)
        if (*c == ' ') p->nwords++;
    p->lps = xmalloc((size_t)p->m * sizeof *p->lps);
    kmp_build_lps(p->text, p->m, p->lps);               /* built once, reused per question */
}

static int cmp_phrase(const void *a, const void *b)
{
    const Phrase *x = a, *y = b;
    if (x->m != y->m) return y->m - x->m;               /* longest first */
    return x->topic - y->topic;
}

int engine_load_syllabus(Engine *e, const char *path)
{
    char **lines;
    int n;
    if (!read_lines(e, path, &lines, &n)) return 0;

    /* Pass 1: aliases, so keywords below are normalized the same way as questions. */
    for (int i = 0; i < n; i++) {
        char *s = trim(lines[i]);
        if (!starts_with_ci(s, "ALIAS:")) continue;
        char *eq = strchr(s + 6, '=');
        if (!eq) {
            engine_add_issue(e, "syllabus line %d: ALIAS needs 'word = expansion'", i + 1);
            continue;
        }
        *eq = '\0';
        char key[MAX_NAME], val[MAX_TEXT];
        normalize_text(s + 6, key, sizeof key);
        normalize_text(eq + 1, val, sizeof val);
        if (!*key || !*val || strchr(key, ' ')) {
            engine_add_issue(e, "syllabus line %d: alias key must be one word", i + 1);
            continue;
        }
        int found;
        void *old = hm_get(e->alias, key, &found);
        if (found) free(old);
        hm_put(e->alias, key, xstrdup(val));
    }

    /* Pass 2: units and topics. */
    int unit = 0;
    for (int i = 0; i < n; i++) {
        char *s = trim(lines[i]);
        if (!*s || *s == '#' || starts_with_ci(s, "ALIAS:")) continue;

        if (starts_with_ci(s, "UNIT:")) {
            unit = (int)strtol(s + 5, NULL, 10);
            continue;
        }
        if (!starts_with_ci(s, "TOPIC:")) {
            engine_add_issue(e, "syllabus line %d: expected UNIT:, TOPIC: or ALIAS:", i + 1);
            continue;
        }

        char *f[2];
        int nf = split_fields(s + 6, '|', f, 2);
        char *name = trim(f[0]);
        int ti = add_topic(e, name, unit);
        if (ti == -1) {
            engine_add_issue(e, "syllabus line %d: topic name is empty", i + 1);
            continue;
        }
        if (ti == -2) {
            engine_add_issue(e, "syllabus line %d: duplicate topic '%s' ignored", i + 1, name);
            continue;
        }
        add_keyword(e, ti, name);                       /* the name is a keyword too */
        if (nf == 2) {
            char *p = f[1];
            while (p) {
                char *comma = strchr(p, ',');
                if (comma) *comma = '\0';
                add_keyword(e, ti, trim(p));
                p = comma ? comma + 1 : NULL;
            }
        }
    }
    free_lines(lines, n);

    if (e->nph > 1) qsort(e->ph, (size_t)e->nph, sizeof *e->ph, cmp_phrase);
    if (e->nt == 0) {
        engine_add_issue(e, "syllabus '%s' defines no topics", path);
        return 0;
    }
    return 1;
}

/* ---------- questions ---------- */

/* "Q3" -> 3; "Q1a" -> 1,'a'; "Q3-or" -> 3, or-alternative. */
static int parse_qid(const char *s, int *num, char *sub, int *is_or)
{
    const char *p = s;
    if (*p == 'Q' || *p == 'q') p++;
    if (!isdigit((unsigned char)*p)) return 0;
    *num = 0;
    while (isdigit((unsigned char)*p)) {
        *num = *num * 10 + (*p - '0');
        if (*num > 999) return 0;
        p++;
    }
    *sub = 0;
    *is_or = 0;
    if (isalpha((unsigned char)*p)) {
        *sub = (char)tolower((unsigned char)*p);
        p++;
    }
    if (*p == '-') {
        if (!str_ieq(p, "-or")) return 0;
        *is_or = 1;
        p += 3;
    }
    return *p == '\0';
}

/* Empty field means MISSING (-1). atoi("") would silently return 0. */
static int parse_marks(const char *s, int *ok)
{
    *ok = 1;
    if (*s == '\0') return -1;
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v <= 0 || v > 100) {
        *ok = 0;
        return -1;
    }
    return (int)v;
}

int engine_load_questions(Engine *e, const char *path)
{
    char **lines;
    int n;
    if (!read_lines(e, path, &lines, &n)) return 0;

    int year = -1;
    for (int i = 0; i < n; i++) {
        int ln = i + 1;
        char *s = trim(lines[i]);
        if (!*s || *s == '#') continue;

        if (starts_with_ci(s, "YEAR:")) {
            char *ys = trim(s + 5), *end;
            long y = strtol(ys, &end, 10);
            if (end == ys || *end != '\0' || y < 1900 || y > 2100) {
                engine_add_issue(e, "line %d: invalid year '%s'; questions below have no year", ln, ys);
                year = -1;
            } else {
                year = (int)y;
            }
            continue;
        }

        char *f[3];
        int nf = split_fields(s, '|', f, 3);
        if (nf < 3) {
            e->n_malformed++;
            engine_add_issue(e, "line %d: expected 'QID | marks | text', found %d field(s)", ln, nf);
            continue;
        }
        char *qid = trim(f[0]), *ms = trim(f[1]), *text = trim(f[2]);
        if (!*text) {
            e->n_malformed++;
            engine_add_issue(e, "line %d: question text is empty", ln);
            continue;
        }

        GROW(e->q, e->nq, e->capq);
        Question *q = &e->q[e->nq++];
        memset(q, 0, sizeof *q);
        q->line_no = ln;
        q->year = year;
        q->topic = -1;
        q->group = -1;
        snprintf(q->qid, sizeof q->qid, "%s", qid);
        if (!parse_qid(qid, &q->qnum, &q->sub, &q->is_or)) {
            q->qnum = -1;
            engine_add_issue(e, "line %d: unrecognised question id '%s'", ln, qid);
        }
        int ok;
        q->marks = parse_marks(ms, &ok);
        if (!ok) engine_add_issue(e, "line %d (%s): invalid marks '%s', treated as missing", ln, q->qid, ms);

        q->raw = xstrdup(text);
        char norm[MAX_TEXT];
        engine_normalize(e, text, norm, sizeof norm);
        q->norm = xstrdup(norm);
    }
    free_lines(lines, n);
    return 1;
}

/* ---------- prerequisites: "Topic A -> Topic B" ---------- */

int engine_load_prereqs(Engine *e, const char *path)
{
    if (!e->prereq) e->prereq = graph_create(e->nt);
    char **lines;
    int n;
    if (!read_lines(e, path, &lines, &n)) return 0;

    for (int i = 0; i < n; i++) {
        char *s = trim(lines[i]);
        if (!*s || *s == '#') continue;
        char *arrow = strstr(s, "->");
        if (!arrow) {
            engine_add_issue(e, "prereqs line %d: expected 'Topic A -> Topic B'", i + 1);
            continue;
        }
        *arrow = '\0';
        char *left = trim(s), *right = trim(arrow + 2);
        char a[MAX_TEXT], b[MAX_TEXT];
        engine_normalize(e, left, a, sizeof a);
        engine_normalize(e, right, b, sizeof b);
        int ia = hm_get_int(e->topic_index, a, -1);
        int ib = hm_get_int(e->topic_index, b, -1);
        if (ia < 0) engine_add_issue(e, "prereqs line %d: unknown topic '%s'", i + 1, left);
        if (ib < 0) engine_add_issue(e, "prereqs line %d: unknown topic '%s'", i + 1, right);
        if (ia < 0 || ib < 0) continue;
        if (ia == ib) {
            engine_add_issue(e, "prereqs line %d: topic cannot require itself", i + 1);
            continue;
        }
        graph_add_edge(e->prereq, ia, ib);
    }
    free_lines(lines, n);
    return 1;
}
