#pragma once

#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

#include "engine.h"   








class AnalyzerEngine
{
public:
    AnalyzerEngine();
    ~AnalyzerEngine();
    AnalyzerEngine(const AnalyzerEngine &) = delete;
    AnalyzerEngine &operator=(const AnalyzerEngine &) = delete;

    
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

    QVector<int> pendingReview() const;          
    QVector<int> ranking() const;                
    QVector<int> studyOrder() const;             
    bool studyOrderComplete() const { return e->nstudy == e->nt; }
    QVector<QPair<int, int>> prerequisitesOf(int topic) const; 
    QVector<QPair<int, int>> dependentsOf(int topic) const;    
    QVector<QVector<int>> repeatGroups() const;  
    QStringList issues() const;

    
    void assignTopic(int question, int topic);

    
    
    void setWeights(double marks, double coverage, double recency, double repeats);
    QVector<double> weights() const;
    static QVector<double> defaultWeights() { return {0.35, 0.25, 0.20, 0.20}; }

    QVector<QPair<int, int>> prerequisiteEdges() const;   

    double backtestScore() const { return bt; }
    double backtestBaseline() const { return bb; }
    static constexpr int kBacktestTopK = 3;

private:
    void recompute();
    void runBacktest();
    QVector<QPair<int, int>> bfsLevels(const Graph *g, int start) const;

    Engine *e;
    Graph *rev = nullptr;   
    bool loaded = false;
    double bt = -1.0;
    double bb = -1.0;
};
