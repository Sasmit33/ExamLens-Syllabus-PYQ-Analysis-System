#include "analyzerengine.h"

#include <algorithm>

AnalyzerEngine::AnalyzerEngine() : e(engine_create()) {}

AnalyzerEngine::~AnalyzerEngine()
{
    graph_free(rev);
    engine_free(e);
}

bool AnalyzerEngine::load(const QString &syllabus, const QString &questions,
                          const QString &prereqs, QString *error)
{
    // Start from a fresh engine so "Run" can be pressed again with other files.
    graph_free(rev);
    rev = nullptr;
    engine_free(e);
    e = engine_create();
    loaded = false;
    bt = bb = -1.0;

    // C functions need char*; toLocal8Bit handles Windows paths.
    const QByteArray s = syllabus.toLocal8Bit();
    const QByteArray q = questions.toLocal8Bit();
    const QByteArray p = prereqs.toLocal8Bit();

    bool ok = engine_load_syllabus(e, s.constData()) &&   // syllabus FIRST: defines topics
              engine_load_questions(e, q.constData());
    if (ok && !prereqs.isEmpty()) ok = engine_load_prereqs(e, p.constData());
    if (!ok) {
        if (error) *error = issues().join("\n");
        return false;
    }

    // Same pipeline order as the command-line main.c
    engine_validate(e);
    engine_map_topics(e);
    engine_detect_repetition(e);
    if (!e->prereq) e->prereq = graph_create(e->nt);
    rev = graph_reverse(e->prereq);
    runBacktest();                 // also recomputes analytics + study order
    loaded = true;
    return true;
}

void AnalyzerEngine::recompute()
{
    engine_compute_analytics(e, e->max_year);
    engine_study_order(e);
}

void AnalyzerEngine::runBacktest()
{
    bt = bb = -1.0;
    if (e->max_year > e->min_year)
        bt = engine_backtest(e, e->max_year, kBacktestTopK, &bb);
    recompute();                   // backtest leaves analytics on the training years
}

QString AnalyzerEngine::topicName(int t) const
{
    if (t < 0 || t >= e->nt) return QStringLiteral("(no topic)");
    return QString::fromUtf8(e->t[t].name);
}

QVector<int> AnalyzerEngine::pendingReview() const
{
    QVector<int> out;
    for (int i = 0; i < e->review.size; i++) {
        int qi = iq_get(&e->review, i);
        if (e->q[qi].needs_review) out.append(qi);    // skip ones already resolved
    }
    return out;
}

QVector<int> AnalyzerEngine::ranking() const
{
    QVector<int> out(e->nt);
    if (e->nt > 0) engine_rank_topics(e, 0, out.data());
    return out;
}

QVector<int> AnalyzerEngine::studyOrder() const
{
    QVector<int> out;
    for (int i = 0; i < e->nstudy; i++) out.append(e->study_order[i]);
    return out;
}

QVector<QPair<int, int>> AnalyzerEngine::bfsLevels(const Graph *g, int start) const
{
    QVector<QPair<int, int>> out;
    if (!g || start < 0 || start >= e->nt) return out;
    QVector<int> level(e->nt);
    graph_bfs(g, start, level.data());
    for (int t = 0; t < e->nt; t++)
        if (level[t] > 0) out.append({t, level[t]});
    std::sort(out.begin(), out.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
        return a.second != b.second ? a.second < b.second : a.first < b.first;
    });
    return out;
}

QVector<QPair<int, int>> AnalyzerEngine::prerequisitesOf(int topic) const
{
    return bfsLevels(rev, topic);          // walk arrows backwards
}

QVector<QPair<int, int>> AnalyzerEngine::dependentsOf(int topic) const
{
    return bfsLevels(e->prereq, topic);    // walk arrows forwards
}

QVector<QVector<int>> AnalyzerEngine::repeatGroups() const
{
    QVector<QVector<int>> groups;
    for (int g = 0; g < e->ngroups; g++) {
        if (e->group_years[g] < 2) continue;
        QVector<int> members;
        for (int i = 0; i < e->nq; i++)
            if (e->q[i].group == g) members.append(i);
        groups.append(members);
    }
    return groups;
}

QStringList AnalyzerEngine::issues() const
{
    QStringList out;
    for (int i = 0; i < e->nissues; i++) out << QString::fromUtf8(e->issues[i]);
    out.removeDuplicates();                // a cycle is re-reported on each recompute
    return out;
}

void AnalyzerEngine::assignTopic(int question, int topic)
{
    engine_set_topic(e, question, topic);
    runBacktest();                         // priorities and study order change
}
