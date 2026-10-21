#pragma once

#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

#include "engine.h"   // C backend; its extern "C" guards make it callable from C++

/*
 * AnalyzerEngine wraps the C Engine in a C++ class.
 *  - RAII: the constructor creates the engine, the destructor frees it,
 *    so memory is released automatically even on early returns.
 *  - Copying is disabled: two objects must never free the same Engine.
 *  - The UI only talks to this class, never to the C functions directly.
 */
class AnalyzerEngine
{
public:
    AnalyzerEngine();
    ~AnalyzerEngine();
    AnalyzerEngine(const AnalyzerEngine &) = delete;
    AnalyzerEngine &operator=(const AnalyzerEngine &) = delete;

    // Runs the whole pipeline. prereqs may be empty. On failure, error gets the reason.
    bool load(const QString &syllabus, const QString &questions,
              const QString &prereqs, QString *error);
    bool isLoaded() const { return loaded; }

    int questionCount() const { return e->nq; }
    const Question &question(int i) const { return e->q[i]; }
    int topicCount() const { return e->nt; }
    const Topic &topic(int i) const { return e->t[i]; }
    QString topicName(int t) const;
    int minYear() const { return e->min_year; }
    int maxYear() const { return e->max_year; }

    QVector<int> pendingReview() const;          // review queue (FIFO), unresolved only
    QVector<int> ranking() const;                // topic ids, highest priority first
    QVector<int> studyOrder() const;             // topological order with priority heap
    bool studyOrderComplete() const { return e->nstudy == e->nt; }
    QVector<QPair<int, int>> prerequisitesOf(int topic) const; // (topic, steps back)
    QVector<QPair<int, int>> dependentsOf(int topic) const;    // (topic, steps ahead)
    QVector<QVector<int>> repeatGroups() const;  // question ids, groups spanning 2+ years
    QStringList issues() const;

    // Human review decision: topic = -1 means "no topic". Recomputes everything after.
    void assignTopic(int question, int topic);

    double backtestScore() const { return bt; }
    double backtestBaseline() const { return bb; }
    static constexpr int kBacktestTopK = 3;

private:
    void recompute();
    void runBacktest();
    QVector<QPair<int, int>> bfsLevels(const Graph *g, int start) const;

    Engine *e;
    Graph *rev = nullptr;   // reversed prerequisite graph, for "what comes before X"
    bool loaded = false;
    double bt = -1.0;
    double bb = -1.0;
};
