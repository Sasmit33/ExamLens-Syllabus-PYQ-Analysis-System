#include "analyzerengine.h"

#include <algorithm>

AnalyzerEngine::AnalyzerEngine() : e(engine_create()) {}

AnalyzerEngine::~AnalyzerEngine()
{
    graph_free(rev);
    engine_free(e);
}

void AnalyzerEngine::setWeights(double marks, double coverage, double recency, double repeats)
{
    engine_set_weights(e, marks, coverage, recency, repeats);
    if (loaded) runBacktest();             
}

QVector<double> AnalyzerEngine::weights() const
{
    return {e->weight[0], e->weight[1], e->weight[2], e->weight[3]};
}

QVector<QPair<int, int>> AnalyzerEngine::prerequisiteEdges() const
{
    QVector<QPair<int, int>> out;
    if (!e->prereq) return out;
    for (int u = 0; u < e->prereq->n; u++)
        for (int k = 0; k < e->prereq->deg[u]; k++) out.append({u, e->prereq->adj[u][k]});
    return out;
}

bool AnalyzerEngine::load(const QString &syllabus, const QString &questions,
                          const QString &prereqs, QString *error)
{
    
    const QVector<double> w = weights();   
    graph_free(rev);
    rev = nullptr;
    engine_free(e);
    e = engine_create();
    engine_set_weights(e, w[0], w[1], w[2], w[3]);
    loaded = false;
    bt = bb = -1.0;

    
    const QByteArray s = syllabus.toLocal8Bit();
    const QByteArray q = questions.toLocal8Bit();
    const QByteArray p = prereqs.toLocal8Bit();

    bool ok = engine_load_syllabus(e, s.constData()) &&   
              engine_load_questions(e, q.constData());
    if (ok && !prereqs.isEmpty()) ok = engine_load_prereqs(e, p.constData());
    if (!ok) {
        if (error) *error = issues().join("\n");
        return false;
    }

    
    engine_validate(e);
    engine_map_topics(e);
    engine_detect_repetition(e);
    if (!e->prereq) e->prereq = graph_create(e->nt);
    rev = graph_reverse(e->prereq);
    runBacktest();                 
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
    recompute();                   
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
        if (e->q[qi].needs_review) out.append(qi);    
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
    return bfsLevels(rev, topic);          
}

QVector<QPair<int, int>> AnalyzerEngine::dependentsOf(int topic) const
{
    return bfsLevels(e->prereq, topic);    
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
    out.removeDuplicates();                
    return out;
}

void AnalyzerEngine::assignTopic(int question, int topic)
{
    engine_set_topic(e, question, topic);
    runBacktest();                         
}
