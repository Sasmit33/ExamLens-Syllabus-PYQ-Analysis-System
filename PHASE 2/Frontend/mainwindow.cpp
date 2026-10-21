#include "mainwindow.h"

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

// Table cell that sorts by number, not by text ("10" after "9").
// Inheritance + overriding a virtual function: a small OOP example.
class NumberItem : public QTableWidgetItem
{
public:
    NumberItem(double value, const QString &text) : QTableWidgetItem(text)
    {
        setData(Qt::UserRole, value);
        setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
    bool operator<(const QTableWidgetItem &other) const override
    {
        return data(Qt::UserRole).toDouble() < other.data(Qt::UserRole).toDouble();
    }
};

const QColor kReviewTint(239, 159, 39, 60);   // amber, readable in light and dark themes
const int kTextRole = Qt::UserRole + 1;

QLabel *boldLabel(const QString &text)
{
    auto *l = new QLabel(text);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    return l;
}

void setupTable(QTableWidget *t, const QStringList &headers)
{
    t->setColumnCount(headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
}

} // namespace

// ---------------------------------------------------------------- construction

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Exam Question Analyzer");

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(buildFileBar());

    tabs = new QTabWidget;
    tabs->addTab(buildQuestionsTab(), "Questions");
    tabs->addTab(buildReviewTab(), "Review queue");
    tabs->addTab(buildPriorityTab(), "Priority");
    tabs->addTab(buildStudyTab(), "Study plan");
    tabs->addTab(buildRepeatsTab(), "Repeated questions");
    tabs->addTab(buildIssuesTab(), "Data issues");
    tabs->setEnabled(false);                       // nothing to show until Run
    layout->addWidget(tabs, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Choose the input files and press Run analysis.");
}

QWidget *MainWindow::buildFileBar()
{
    auto *box = new QWidget;
    auto *grid = new QGridLayout(box);
    grid->setContentsMargins(0, 0, 0, 0);

    struct Row { const char *label; const char *file; QLineEdit **edit; };
    const Row rows[] = {
        {"Syllabus", "syllabus.txt", &syllabusEdit},
        {"Questions", "questions.txt", &questionsEdit},
        {"Prerequisites (optional)", "prereqs.txt", &prereqsEdit},
    };
    int r = 0;
    for (const Row &row : rows) {
        auto *edit = new QLineEdit(findDefault(row.file));
        *row.edit = edit;
        auto *browse = new QPushButton("Browse");
        const QString title = QStringLiteral("Select %1 file").arg(QString::fromLatin1(row.label));
        connect(browse, &QPushButton::clicked, this, [this, edit, title] { browseInto(edit, title); });
        grid->addWidget(new QLabel(row.label), r, 0);
        grid->addWidget(edit, r, 1);
        grid->addWidget(browse, r, 2);
        r++;
    }

    auto *run = new QPushButton("Run analysis");
    run->setMinimumHeight(64);
    connect(run, &QPushButton::clicked, this, &MainWindow::runAnalysis);
    grid->addWidget(run, 0, 3, 3, 1);
    grid->setColumnStretch(1, 1);
    return box;
}

QWidget *MainWindow::buildQuestionsTab()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);

    auto *filters = new QHBoxLayout;
    searchEdit = new QLineEdit;
    searchEdit->setPlaceholderText("Search questions, topics or ids");
    statusFilter = new QComboBox;
    statusFilter->addItems({"All questions", "Needs review", "No topic", "Missing marks"});
    filters->addWidget(searchEdit, 1);
    filters->addWidget(statusFilter);
    v->addLayout(filters);

    questionsTable = new QTableWidget;
    setupTable(questionsTable, {"Year", "Id", "Marks", "Topic", "Confidence", "Status", "Question"});
    v->addWidget(questionsTable, 1);

    connect(searchEdit, &QLineEdit::textChanged, this, &MainWindow::applyQuestionFilter);
    connect(statusFilter, &QComboBox::currentIndexChanged, this, &MainWindow::applyQuestionFilter);
    return page;
}

QWidget *MainWindow::buildReviewTab()
{
    auto *split = new QSplitter(Qt::Horizontal);

    auto *left = new QWidget;
    auto *lv = new QVBoxLayout(left);
    reviewCount = boldLabel("");
    reviewList = new QListWidget;
    lv->addWidget(reviewCount);
    lv->addWidget(reviewList, 1);

    auto *right = new QWidget;
    auto *rv = new QVBoxLayout(right);
    rv->addWidget(boldLabel("Question"));
    reviewText = new QLabel;
    reviewText->setWordWrap(true);
    reviewText->setTextInteractionFlags(Qt::TextSelectableByMouse);
    rv->addWidget(reviewText);
    reviewGuess = new QLabel;
    reviewGuess->setWordWrap(true);
    rv->addWidget(reviewGuess);

    auto *pick = new QHBoxLayout;
    pick->addWidget(new QLabel("Correct topic:"));
    topicCombo = new QComboBox;
    pick->addWidget(topicCombo, 1);
    rv->addLayout(pick);

    auto *buttons = new QHBoxLayout;
    assignButton = new QPushButton("Assign topic");
    acceptButton = new QPushButton("Accept best guess");
    noTopicButton = new QPushButton("No topic");
    buttons->addWidget(assignButton);
    buttons->addWidget(acceptButton);
    buttons->addWidget(noTopicButton);
    rv->addLayout(buttons);

    auto *hint = new QLabel("Questions are reviewed in the order they were flagged (FIFO). "
                            "Each decision immediately updates priorities and the study plan.");
    hint->setWordWrap(true);
    rv->addWidget(hint);
    rv->addStretch(1);

    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 2);

    connect(reviewList, &QListWidget::currentRowChanged, this, &MainWindow::onReviewSelectionChanged);
    connect(assignButton, &QPushButton::clicked, this, &MainWindow::assignSelectedTopic);
    connect(acceptButton, &QPushButton::clicked, this, &MainWindow::acceptGuess);
    connect(noTopicButton, &QPushButton::clicked, this, &MainWindow::markNoTopic);
    return split;
}

QWidget *MainWindow::buildPriorityTab()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    backtestLabel = new QLabel;
    backtestLabel->setWordWrap(true);
    v->addWidget(backtestLabel);

    priorityTable = new QTableWidget;
    setupTable(priorityTable, {"Rank", "Topic", "Priority", "Marks", "Years asked", "Last asked", "Repeats"});
    priorityTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    priorityTable->setColumnWidth(2, 220);
    v->addWidget(priorityTable, 1);
    v->addWidget(new QLabel("Double-click a topic to see what you need to study before it."));

    connect(priorityTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::openTopicInStudyPlan);
    return page;
}

QWidget *MainWindow::buildStudyTab()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    cycleWarning = new QLabel;
    cycleWarning->setWordWrap(true);
    cycleWarning->setStyleSheet("color: #A32D2D;");
    cycleWarning->hide();
    v->addWidget(cycleWarning);

    auto *split = new QSplitter(Qt::Horizontal);
    auto *left = new QWidget;
    auto *lv = new QVBoxLayout(left);
    lv->addWidget(boldLabel("Study order (prerequisites first, then priority)"));
    studyList = new QListWidget;
    lv->addWidget(studyList, 1);

    auto *right = new QWidget;
    auto *rv = new QVBoxLayout(right);
    studyTitle = boldLabel("Select a topic");
    rv->addWidget(studyTitle);
    prereqTree = new QTreeWidget;
    prereqTree->setHeaderLabels({"Study these first (BFS, backwards)"});
    unlockTree = new QTreeWidget;
    unlockTree->setHeaderLabels({"This topic leads to (BFS, forwards)"});
    rv->addWidget(prereqTree, 1);
    rv->addWidget(unlockTree, 1);

    split->addWidget(left);
    split->addWidget(right);
    v->addWidget(split, 1);

    connect(studyList, &QListWidget::currentRowChanged, this, &MainWindow::onStudyTopicSelected);
    return page;
}

QWidget *MainWindow::buildRepeatsTab()
{
    repeatTree = new QTreeWidget;
    repeatTree->setHeaderLabels({"Year", "Id", "Question"});
    repeatTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return repeatTree;
}

QWidget *MainWindow::buildIssuesTab()
{
    issueList = new QListWidget;
    issueList->setWordWrap(true);
    return issueList;
}

// ---------------------------------------------------------------- actions

QString MainWindow::findDefault(const QString &fileName)
{
    // Look upwards from the .exe for exam_analyzer/data/<file> (handy while developing).
    QDir dir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; depth++) {
        for (const QString &rel : {QString("exam_analyzer/data/%1"), QString("data/%1")}) {
            const QString candidate = dir.filePath(rel.arg(fileName));
            if (QFileInfo::exists(candidate)) return QDir::toNativeSeparators(candidate);
        }
        if (!dir.cdUp()) break;
    }
    return QString();
}

void MainWindow::browseInto(QLineEdit *target, const QString &title)
{
    const QString start = target->text().isEmpty() ? QDir::homePath()
                                                   : QFileInfo(target->text()).absolutePath();
    const QString path = QFileDialog::getOpenFileName(this, title, start,
                                                      "Text files (*.txt);;All files (*)");
    if (!path.isEmpty()) target->setText(QDir::toNativeSeparators(path));
}

void MainWindow::runAnalysis()
{
    const QString syllabus = syllabusEdit->text().trimmed();
    const QString questions = questionsEdit->text().trimmed();
    const QString prereqs = prereqsEdit->text().trimmed();
    if (syllabus.isEmpty() || questions.isEmpty()) {
        QMessageBox::warning(this, "Missing files", "Choose both a syllabus file and a questions file.");
        return;
    }

    QString error;
    if (!engine.load(syllabus, questions, prereqs, &error)) {
        tabs->setEnabled(false);
        QMessageBox::critical(this, "Could not load files", error);
        return;
    }

    topicCombo->clear();
    for (int t = 0; t < engine.topicCount(); t++) topicCombo->addItem(engine.topicName(t), t);

    tabs->setEnabled(true);
    refreshAll();
    if (reviewList->count() > 0) reviewList->setCurrentRow(0);
}

void MainWindow::refreshAll()
{
    refreshQuestions();
    refreshReview();
    refreshPriority();
    refreshStudyPlan();
    refreshRepeats();
    refreshIssues();
    refreshSummary();
}

void MainWindow::refreshSummary()
{
    statusBar()->showMessage(QString("%1 questions (%2-%3)  |  %4 topics  |  %5 waiting for review  |  %6 data issues")
                                 .arg(engine.questionCount())
                                 .arg(engine.minYear())
                                 .arg(engine.maxYear())
                                 .arg(engine.topicCount())
                                 .arg(engine.pendingReview().size())
                                 .arg(engine.issues().size()));
}

// ---------------------------------------------------------------- questions tab

void MainWindow::refreshQuestions()
{
    questionsTable->setSortingEnabled(false);
    questionsTable->setRowCount(0);

    for (int i = 0; i < engine.questionCount(); i++) {
        const Question &q = engine.question(i);
        const int r = questionsTable->rowCount();
        questionsTable->insertRow(r);

        QStringList status;
        if (q.is_duplicate) status << "duplicate (ignored)";
        if (q.needs_review) status << "needs review";
        if (q.in_or_pair) status << "OR pair";
        if (q.marks < 0) status << "no marks";

        auto *year = new NumberItem(q.year, q.year < 0 ? "-" : QString::number(q.year));
        year->setData(kTextRole, status.join(", "));
        questionsTable->setItem(r, 0, year);
        questionsTable->setItem(r, 1, new QTableWidgetItem(QString::fromUtf8(q.qid)));
        questionsTable->setItem(r, 2, new NumberItem(q.marks, q.marks < 0 ? "-" : QString::number(q.marks)));
        questionsTable->setItem(r, 3, new QTableWidgetItem(q.is_duplicate ? "-" : engine.topicName(q.topic)));
        questionsTable->setItem(r, 4, new NumberItem(q.confidence, QString::number(q.confidence, 'f', 2)));
        questionsTable->setItem(r, 5, new QTableWidgetItem(status.join(", ")));
        auto *text = new QTableWidgetItem(QString::fromUtf8(q.raw));
        text->setToolTip(QString::fromUtf8(q.raw));
        questionsTable->setItem(r, 6, text);

        if (q.needs_review)
            for (int c = 0; c < questionsTable->columnCount(); c++)
                questionsTable->item(r, c)->setBackground(QBrush(kReviewTint));
    }
    questionsTable->setSortingEnabled(true);
    applyQuestionFilter();
}

void MainWindow::applyQuestionFilter()
{
    const QString needle = searchEdit->text().trimmed();
    const int mode = statusFilter->currentIndex();

    for (int r = 0; r < questionsTable->rowCount(); r++) {
        bool textOk = needle.isEmpty();
        for (int c = 0; c < questionsTable->columnCount() && !textOk; c++)
            textOk = questionsTable->item(r, c)->text().contains(needle, Qt::CaseInsensitive);

        const QString status = questionsTable->item(r, 0)->data(kTextRole).toString();
        const QString topic = questionsTable->item(r, 3)->text();
        bool statusOk = true;
        if (mode == 1) statusOk = status.contains("needs review");
        if (mode == 2) statusOk = topic == "(no topic)";
        if (mode == 3) statusOk = status.contains("no marks");

        questionsTable->setRowHidden(r, !(textOk && statusOk));
    }
}

// ---------------------------------------------------------------- review tab

void MainWindow::refreshReview()
{
    const int keepRow = reviewList->currentRow();
    reviewList->blockSignals(true);
    reviewList->clear();

    const QVector<int> pending = engine.pendingReview();
    for (int qi : pending) {
        const Question &q = engine.question(qi);
        auto *item = new QListWidgetItem(QString("[%1 %2] %3")
                                             .arg(q.year)
                                             .arg(QString::fromUtf8(q.qid))
                                             .arg(QString::fromUtf8(q.raw)));
        item->setData(Qt::UserRole, qi);
        item->setToolTip(QString::fromUtf8(q.raw));
        reviewList->addItem(item);
    }
    reviewList->blockSignals(false);

    reviewCount->setText(pending.isEmpty() ? "Nothing left to review"
                                           : QString("%1 question(s) need review").arg(pending.size()));
    if (reviewList->count() > 0)
        reviewList->setCurrentRow(std::clamp(keepRow, 0, reviewList->count() - 1));
    onReviewSelectionChanged();
}

int MainWindow::currentReviewQuestion() const
{
    const QListWidgetItem *item = reviewList->currentItem();
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void MainWindow::onReviewSelectionChanged()
{
    const int qi = currentReviewQuestion();
    const bool has = qi >= 0;
    assignButton->setEnabled(has);
    acceptButton->setEnabled(has);
    noTopicButton->setEnabled(has);
    topicCombo->setEnabled(has);
    if (!has) {
        reviewText->setText("All flagged questions have been reviewed.");
        reviewGuess->clear();
        return;
    }

    const Question &q = engine.question(qi);
    reviewText->setText(QString("%1 %2 (%3 marks)\n\n%4")
                            .arg(q.year)
                            .arg(QString::fromUtf8(q.qid))
                            .arg(q.marks < 0 ? QString("missing") : QString::number(q.marks))
                            .arg(QString::fromUtf8(q.raw)));
    reviewGuess->setText(QString("Program's best guess: %1   (confidence %2, score %3)")
                             .arg(engine.topicName(q.topic))
                             .arg(q.confidence, 0, 'f', 2)
                             .arg(q.score, 0, 'f', 1));
    const int idx = topicCombo->findData(q.topic);
    if (idx >= 0) topicCombo->setCurrentIndex(idx);
}

void MainWindow::resolveCurrentReview(int topic)
{
    const int qi = currentReviewQuestion();
    if (qi < 0) return;
    const QString id = QString::fromUtf8(engine.question(qi).qid);
    engine.assignTopic(qi, topic);       // C backend: engine_set_topic + recompute
    refreshAll();
    statusBar()->showMessage(QString("%1 assigned to %2. Priorities updated.").arg(id, engine.topicName(topic)), 5000);
}

void MainWindow::assignSelectedTopic() { resolveCurrentReview(topicCombo->currentData().toInt()); }

void MainWindow::acceptGuess()
{
    const int qi = currentReviewQuestion();
    if (qi >= 0) resolveCurrentReview(engine.question(qi).topic);
}

void MainWindow::markNoTopic() { resolveCurrentReview(-1); }

// ---------------------------------------------------------------- priority tab

void MainWindow::refreshPriority()
{
    if (engine.backtestScore() < 0) {
        backtestLabel->setText("Backtest needs questions with marks from at least two different years.");
    } else {
        backtestLabel->setText(QString("Backtest: trained on %1-%2, tested on %3. The top %4 topics by priority "
                                       "cover %5% of %3's marks; ranking by past marks alone covers %6%.")
                                   .arg(engine.minYear())
                                   .arg(engine.maxYear() - 1)
                                   .arg(engine.maxYear())
                                   .arg(AnalyzerEngine::kBacktestTopK)
                                   .arg(qRound(100 * engine.backtestScore()))
                                   .arg(qRound(100 * engine.backtestBaseline())));
    }

    const QVector<int> rank = engine.ranking();
    priorityTable->setRowCount(rank.size());
    for (int r = 0; r < rank.size(); r++) {
        const Topic &t = engine.topic(rank[r]);
        priorityTable->setItem(r, 0, new NumberItem(r + 1, QString::number(r + 1)));
        auto *name = new QTableWidgetItem(engine.topicName(rank[r]));
        name->setData(Qt::UserRole, rank[r]);
        priorityTable->setItem(r, 1, name);

        auto *bar = new QProgressBar;
        bar->setRange(0, 1000);
        bar->setValue(qRound(t.priority * 1000));
        bar->setFormat(QString::number(t.priority, 'f', 3));
        bar->setTextVisible(true);
        priorityTable->setCellWidget(r, 2, bar);

        priorityTable->setItem(r, 3, new NumberItem(t.w_marks, QString::number(t.w_marks, 'f', 1)));
        priorityTable->setItem(r, 4, new NumberItem(t.nyears, QString::number(t.nyears)));
        priorityTable->setItem(r, 5, new NumberItem(t.last_year,
                                                    t.last_year < 0 ? "never" : QString::number(t.last_year)));
        priorityTable->setItem(r, 6, new NumberItem(t.repeat_score, QString::number(t.repeat_score, 'f', 1)));
        if (t.nyears == 0)
            priorityTable->item(r, 1)->setToolTip("Not seen in past papers: still cover the basics.");
    }
}

void MainWindow::openTopicInStudyPlan(int row, int)
{
    const QTableWidgetItem *name = priorityTable->item(row, 1);
    if (!name) return;
    const int topic = name->data(Qt::UserRole).toInt();
    for (int i = 0; i < studyList->count(); i++) {
        if (studyList->item(i)->data(Qt::UserRole).toInt() == topic) {
            studyList->setCurrentRow(i);
            break;
        }
    }
    tabs->setCurrentIndex(3);
}

// ---------------------------------------------------------------- study plan tab

void MainWindow::refreshStudyPlan()
{
    const QListWidgetItem *current = studyList->currentItem();
    const int keepTopic = current ? current->data(Qt::UserRole).toInt() : -1;

    studyList->blockSignals(true);
    studyList->clear();
    const QVector<int> order = engine.studyOrder();
    int keepRow = 0;
    for (int i = 0; i < order.size(); i++) {
        const int t = order[i];
        auto *item = new QListWidgetItem(QString("%1.  %2   (priority %3)")
                                             .arg(i + 1)
                                             .arg(engine.topicName(t))
                                             .arg(engine.topic(t).priority, 0, 'f', 3));
        item->setData(Qt::UserRole, t);
        studyList->addItem(item);
        if (t == keepTopic) keepRow = i;
    }
    studyList->blockSignals(false);

    if (!engine.studyOrderComplete()) {
        cycleWarning->setText(QString("Only %1 of %2 topics could be ordered: the prerequisites contain a cycle. "
                                      "See the Data issues tab.")
                                  .arg(order.size())
                                  .arg(engine.topicCount()));
        cycleWarning->show();
    } else {
        cycleWarning->hide();
    }

    if (studyList->count() > 0) studyList->setCurrentRow(keepRow);
    onStudyTopicSelected();
}

void MainWindow::onStudyTopicSelected()
{
    prereqTree->clear();
    unlockTree->clear();
    const QListWidgetItem *item = studyList->currentItem();
    if (!item) {
        studyTitle->setText("Select a topic");
        return;
    }
    const int topic = item->data(Qt::UserRole).toInt();
    studyTitle->setText(engine.topicName(topic));

    auto fill = [this](QTreeWidget *tree, const QVector<QPair<int, int>> &levels,
                       const QString &stepWord, const QString &emptyText) {
        if (levels.isEmpty()) {
            tree->addTopLevelItem(new QTreeWidgetItem(QStringList{emptyText}));
            return;
        }
        QMap<int, QTreeWidgetItem *> groups;
        for (const auto &pair : levels) {
            if (!groups.contains(pair.second)) {
                auto *g = new QTreeWidgetItem(QStringList{QString("%1 step(s) %2").arg(pair.second).arg(stepWord)});
                tree->addTopLevelItem(g);
                groups.insert(pair.second, g);
            }
            new QTreeWidgetItem(groups.value(pair.second), QStringList{engine.topicName(pair.first)});
        }
        tree->expandAll();
    };
    fill(prereqTree, engine.prerequisitesOf(topic), "back", "No prerequisites: you can start here");
    fill(unlockTree, engine.dependentsOf(topic), "ahead", "No later topics depend on this one");
}

// ---------------------------------------------------------------- repeats and issues

void MainWindow::refreshRepeats()
{
    repeatTree->clear();
    const QVector<QVector<int>> groups = engine.repeatGroups();
    if (groups.isEmpty()) {
        repeatTree->addTopLevelItem(new QTreeWidgetItem(QStringList{"No repeated questions found"}));
        return;
    }
    int n = 1;
    for (const QVector<int> &members : groups) {
        QSet<int> years;
        for (int qi : members) years.insert(engine.question(qi).year);
        auto *group = new QTreeWidgetItem(QStringList{QString("Group %1: asked in %2 years").arg(n++).arg(years.size())});
        repeatTree->addTopLevelItem(group);
        group->setFirstColumnSpanned(true);
        for (int qi : members) {
            const Question &q = engine.question(qi);
            new QTreeWidgetItem(group, QStringList{QString::number(q.year), QString::fromUtf8(q.qid),
                                                   QString::fromUtf8(q.raw)});
        }
    }
    repeatTree->expandAll();
}

void MainWindow::refreshIssues()
{
    issueList->clear();
    const QStringList issues = engine.issues();
    if (issues.isEmpty())
        issueList->addItem("No problems found in the input files.");
    else
        issueList->addItems(issues);
}
