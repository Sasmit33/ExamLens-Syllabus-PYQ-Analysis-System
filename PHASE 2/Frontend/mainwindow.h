#pragma once

#include <QMainWindow>

#include "analyzerengine.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTabWidget;
class QTableWidget;
class QTreeWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void runAnalysis();
    void applyQuestionFilter();
    void onReviewSelectionChanged();
    void assignSelectedTopic();
    void acceptGuess();
    void markNoTopic();
    void onStudyTopicSelected();
    void openTopicInStudyPlan(int row, int column);

private:
    // building the UI
    QWidget *buildFileBar();
    QWidget *buildQuestionsTab();
    QWidget *buildReviewTab();
    QWidget *buildPriorityTab();
    QWidget *buildStudyTab();
    QWidget *buildRepeatsTab();
    QWidget *buildIssuesTab();

    // filling the UI from the engine
    void refreshAll();
    void refreshQuestions();
    void refreshReview();
    void refreshPriority();
    void refreshStudyPlan();
    void refreshRepeats();
    void refreshIssues();
    void refreshSummary();

    void resolveCurrentReview(int topic);
    int currentReviewQuestion() const;
    void browseInto(QLineEdit *target, const QString &title);
    static QString findDefault(const QString &fileName);

    AnalyzerEngine engine;

    QLineEdit *syllabusEdit = nullptr;
    QLineEdit *questionsEdit = nullptr;
    QLineEdit *prereqsEdit = nullptr;
    QTabWidget *tabs = nullptr;

    QLineEdit *searchEdit = nullptr;
    QComboBox *statusFilter = nullptr;
    QTableWidget *questionsTable = nullptr;

    QListWidget *reviewList = nullptr;
    QLabel *reviewCount = nullptr;
    QLabel *reviewText = nullptr;
    QLabel *reviewGuess = nullptr;
    QComboBox *topicCombo = nullptr;
    QPushButton *assignButton = nullptr;
    QPushButton *acceptButton = nullptr;
    QPushButton *noTopicButton = nullptr;

    QLabel *backtestLabel = nullptr;
    QTableWidget *priorityTable = nullptr;

    QLabel *cycleWarning = nullptr;
    QListWidget *studyList = nullptr;
    QLabel *studyTitle = nullptr;
    QTreeWidget *prereqTree = nullptr;
    QTreeWidget *unlockTree = nullptr;

    QTreeWidget *repeatTree = nullptr;
    QListWidget *issueList = nullptr;
};
