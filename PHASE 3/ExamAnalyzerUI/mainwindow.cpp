#include "mainwindow.h"
#include "graphview.h"
#include "pdfimportdialog.h"
#include "prioritychart.h"

#include <QAction>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QKeySequence>
#include <QMimeData>
#include <QPalette>
#include <QScrollArea>
#include <QShortcut>
#include <QSlider>
#include <QStyle>
#include <QToolBar>
#include <QUrl>

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

const QColor kReviewTint(239, 159, 39, 60);   
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

} 



MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Exam Question Analyzer");

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->addWidget(buildFileBar());

    tabs = new QTabWidget;
    tabs->addTab(buildOverviewTab(), "Overview");
    tabs->addTab(buildQuestionsTab(), "Questions");
    reviewTab = buildReviewTab();
    tabs->addTab(reviewTab, "Review queue");
    tabs->addTab(buildPriorityTab(), "Priority");
    studyTab = buildStudyTab();
    tabs->addTab(studyTab, "Study plan");
    tabs->addTab(buildRepeatsTab(), "Repeated questions");
    tabs->addTab(buildIssuesTab(), "Data issues");
    tabs->setEnabled(false);                       
    layout->addWidget(tabs, 1);

    setCentralWidget(central);

    
    QToolBar *bar = addToolBar("Main");
    bar->setMovable(false);
    QAction *runAction = bar->addAction("Run analysis");
    runAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(runAction, &QAction::triggered, this, &MainWindow::runAnalysis);
    QAction *pdfAction = bar->addAction("Import PDFs");
    pdfAction->setShortcut(QKeySequence("Ctrl+I"));
    connect(pdfAction, &QAction::triggered, this, [this] { importPdfs(); });
    QAction *darkAction = bar->addAction("Dark mode");
    darkAction->setCheckable(true);
    darkAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(darkAction, &QAction::toggled, this, &MainWindow::setDarkMode);

    
    auto onReviewTab = [this] { return tabs->currentWidget() == reviewTab; };
    connect(new QShortcut(QKeySequence("Ctrl+Return"), this), &QShortcut::activated, this,
            [this, onReviewTab] { if (onReviewTab()) assignSelectedTopic(); });
    connect(new QShortcut(QKeySequence("Ctrl+G"), this), &QShortcut::activated, this,
            [this, onReviewTab] { if (onReviewTab()) acceptGuess(); });
    connect(new QShortcut(QKeySequence("Ctrl+N"), this), &QShortcut::activated, this,
            [this, onReviewTab] { if (onReviewTab()) markNoTopic(); });

    setAcceptDrops(true);    
    refreshOverview();
    statusBar()->showMessage("Choose the input files (or drag them onto this window) and press Run analysis.");
}



QWidget *MainWindow::buildOverviewTab()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);

    
    auto *cards = new QHBoxLayout;
    const char *titles[5] = {"Questions", "Topics", "Waiting for review", "Repeated questions", "Data issues"};
    for (int i = 0; i < 5; i++) {
        auto *card = new QFrame;
        card->setObjectName("card");
        card->setStyleSheet("QFrame#card { border: 1px solid palette(mid); border-radius: 8px; }");
        auto *cl = new QVBoxLayout(card);
        cardValue[i] = new QLabel("-");
        QFont f = cardValue[i]->font();
        f.setPointSizeF(f.pointSizeF() * 1.9);
        f.setBold(true);
        cardValue[i]->setFont(f);
        cl->addWidget(cardValue[i]);
        cl->addWidget(new QLabel(titles[i]));
        cards->addWidget(card);
    }
    v->addLayout(cards);

    auto *split = new QSplitter(Qt::Horizontal);

    
    auto *left = new QWidget;
    auto *lv = new QVBoxLayout(left);
    lv->addWidget(boldLabel("Topic priority (click a bar to open its study path)"));
    overviewChart = new PriorityChart;
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(overviewChart);
    lv->addWidget(scroll, 1);

    
    auto *right = new QWidget;
    auto *rv = new QVBoxLayout(right);
    rv->addWidget(boldLabel("What-if: change the priority weights"));
    auto *hint = new QLabel("Move a slider to see how the ranking, study plan and backtest change. "
                            "Weights are scaled so they add up to 100%.");
    hint->setWordWrap(true);
    rv->addWidget(hint);

    const char *names[4] = {"Marks", "Years asked", "Recency", "Repeats"};
    const QVector<double> w = AnalyzerEngine::defaultWeights();
    auto *grid = new QGridLayout;
    for (int i = 0; i < 4; i++) {
        weightSlider[i] = new QSlider(Qt::Horizontal);
        weightSlider[i]->setRange(0, 100);
        weightSlider[i]->setValue(qRound(w[i] * 100));
        weightValue[i] = new QLabel;
        weightValue[i]->setMinimumWidth(44);
        grid->addWidget(new QLabel(names[i]), i, 0);
        grid->addWidget(weightSlider[i], i, 1);
        grid->addWidget(weightValue[i], i, 2);
        connect(weightSlider[i], &QSlider::valueChanged, this, &MainWindow::onWeightsChanged);
    }
    rv->addLayout(grid);

    auto *reset = new QPushButton("Reset to default weights");
    connect(reset, &QPushButton::clicked, this, &MainWindow::resetWeights);
    rv->addWidget(reset);

    whatIfLabel = new QLabel;
    whatIfLabel->setWordWrap(true);
    rv->addWidget(whatIfLabel);
    rv->addStretch(1);

    split->addWidget(left);
    split->addWidget(right);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    v->addWidget(split, 1);

    connect(overviewChart, &PriorityChart::topicClicked, this, &MainWindow::showTopicInStudyPlan);
    return page;
}

void MainWindow::refreshOverview()
{
    const QVector<double> w = engine.weights();
    for (int i = 0; i < 4; i++) weightValue[i]->setText(QString("%1%").arg(qRound(w[i] * 100)));
    if (!engine.isLoaded()) return;

    cardValue[0]->setText(QString::number(engine.questionCount()));
    cardValue[1]->setText(QString::number(engine.topicCount()));
    cardValue[2]->setText(QString::number(engine.pendingReview().size()));
    cardValue[3]->setText(QString::number(engine.repeatGroups().size()));
    cardValue[4]->setText(QString::number(engine.issues().size()));

    QVector<PriorityChart::Bar> bars;
    for (int t : engine.ranking()) {
        const Topic &tp = engine.topic(t);
        const QString last = tp.last_year < 0 ? QString("never") : QString::number(tp.last_year);
        bars.append({t, engine.topicName(t), tp.priority,
                     QString("%1\nPriority %2\nMarks %3, asked in %4 year(s), last asked %5\nClick to open its study path")
                         .arg(engine.topicName(t))
                         .arg(tp.priority, 0, 'f', 3)
                         .arg(tp.w_marks, 0, 'f', 1)
                         .arg(tp.nyears)
                         .arg(last)});
    }
    overviewChart->setBars(bars);

    if (engine.backtestScore() < 0) {
        whatIfLabel->setText("Backtest needs questions with marks from at least two years.");
    } else {
        whatIfLabel->setText(QString("<b>Backtest with these weights:</b> the top %1 topics would have covered "
                                     "<b>%2%</b> of %3's marks (ranking by past marks alone: %4%).")
                                 .arg(AnalyzerEngine::kBacktestTopK)
                                 .arg(qRound(100 * engine.backtestScore()))
                                 .arg(engine.maxYear())
                                 .arg(qRound(100 * engine.backtestBaseline())));
    }
}

void MainWindow::onWeightsChanged()
{
    if (settingSliders) return;
    engine.setWeights(weightSlider[0]->value(), weightSlider[1]->value(),
                      weightSlider[2]->value(), weightSlider[3]->value());
    if (engine.isLoaded()) {
        refreshPriority();
        refreshStudyPlan();
        refreshSummary();
    }
    refreshOverview();
}

void MainWindow::resetWeights()
{
    const QVector<double> w = AnalyzerEngine::defaultWeights();
    settingSliders = true;                       
    for (int i = 0; i < 4; i++) weightSlider[i]->setValue(qRound(w[i] * 100));
    settingSliders = false;
    onWeightsChanged();
}

void MainWindow::setDarkMode(bool on)
{
    QPalette p = QApplication::style()->standardPalette();
    if (on) {
        const QColor text(230, 233, 240);
        p.setColor(QPalette::Window, QColor(30, 34, 42));
        p.setColor(QPalette::WindowText, text);
        p.setColor(QPalette::Base, QColor(22, 25, 32));
        p.setColor(QPalette::AlternateBase, QColor(36, 40, 50));
        p.setColor(QPalette::Text, text);
        p.setColor(QPalette::Button, QColor(44, 49, 60));
        p.setColor(QPalette::ButtonText, text);
        p.setColor(QPalette::Highlight, QColor(94, 129, 196));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::ToolTipBase, QColor(44, 49, 60));
        p.setColor(QPalette::ToolTipText, text);
        p.setColor(QPalette::Mid, QColor(80, 86, 100));
        p.setColor(QPalette::PlaceholderText, QColor(140, 146, 160));
    }
    QApplication::setPalette(p);
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // PDFs go to the import window; .txt files load directly as before
    QStringList pdfs;
    for (const QUrl &url : event->mimeData()->urls())
        if (url.toLocalFile().endsWith(".pdf", Qt::CaseInsensitive)) pdfs << url.toLocalFile();
    if (!pdfs.isEmpty()) {
        event->acceptProposedAction();
        importPdfs(pdfs);
        return;
    }

    int matched = 0;
    for (const QUrl &url : event->mimeData()->urls()) {
        const QString path = QDir::toNativeSeparators(url.toLocalFile());
        const QString name = QFileInfo(path).fileName().toLower();
        if (name.contains("syllabus")) { syllabusEdit->setText(path); matched++; }
        else if (name.contains("prereq")) { prereqsEdit->setText(path); matched++; }
        else if (name.endsWith(".txt")) { questionsEdit->setText(path); matched++; }
    }
    if (matched == 0) {
        statusBar()->showMessage("Drop PDFs, or .txt files such as syllabus.txt, questions.txt, prereqs.txt", 5000);
        return;
    }
    event->acceptProposedAction();
    if (!syllabusEdit->text().isEmpty() && !questionsEdit->text().isEmpty()) runAnalysis();
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

    // Row 4: build the two files above from PDFs
    auto *pdfHint = new QLabel("Have PDFs? Convert a syllabus PDF and question paper PDFs into the files above.");
    pdfHint->setWordWrap(true);
    auto *importButton = new QPushButton("Import PDFs...");
    connect(importButton, &QPushButton::clicked, this, [this] { importPdfs(); });
    grid->addWidget(new QLabel("From PDFs"), r, 0);
    grid->addWidget(pdfHint, r, 1);
    grid->addWidget(importButton, r, 2);

    auto *run = new QPushButton("Run analysis");
    run->setMinimumHeight(64);
    connect(run, &QPushButton::clicked, this, &MainWindow::runAnalysis);
    grid->addWidget(run, 0, 3, 4, 1);
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
    assignButton->setToolTip("Ctrl+Enter");
    acceptButton->setToolTip("Ctrl+G");
    noTopicButton->setToolTip("Ctrl+N");
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

    
    graphView = new GraphView;
    auto *graphScroll = new QScrollArea;
    graphScroll->setWidgetResizable(true);
    graphScroll->setWidget(graphView);
    auto *graphBox = new QWidget;
    auto *gv = new QVBoxLayout(graphBox);
    gv->setContentsMargins(0, 0, 0, 0);
    gv->addWidget(boldLabel("Prerequisite graph: click a topic (red = selected, orange = study first, green = leads to)"));
    gv->addWidget(graphScroll, 1);

    auto *vsplit = new QSplitter(Qt::Vertical);
    vsplit->addWidget(graphBox);
    vsplit->addWidget(split);
    vsplit->setStretchFactor(0, 3);
    vsplit->setStretchFactor(1, 2);
    v->addWidget(vsplit, 1);

    connect(studyList, &QListWidget::currentRowChanged, this, &MainWindow::onStudyTopicSelected);
    connect(graphView, &GraphView::topicClicked, this, &MainWindow::showTopicInStudyPlan);
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



QString MainWindow::findDefault(const QString &fileName)
{
    
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

void MainWindow::importPdfs(const QStringList &dropped)
{
    PdfImportDialog dialog(this);

    // Dropped files: the one named like "syllabus" is the syllabus, the rest are papers
    QStringList papers;
    for (const QString &path : dropped) {
        if (QFileInfo(path).fileName().contains("syllabus", Qt::CaseInsensitive))
            dialog.setSyllabusPdf(path);
        else
            papers << path;
    }
    dialog.addPaperPdfs(papers);

    if (dialog.exec() != QDialog::Accepted) return;

    syllabusEdit->setText(dialog.syllabusTxt());
    questionsEdit->setText(dialog.questionsTxt());
    prereqsEdit->clear();   // sample prereqs use other topic names; add your own if needed
    runAnalysis();
    statusBar()->showMessage("Loaded from PDFs. Converted files: " +
                             QFileInfo(dialog.syllabusTxt()).absolutePath(), 8000);
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
    refreshOverview();
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
    engine.assignTopic(qi, topic);       
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
    showTopicInStudyPlan(name->data(Qt::UserRole).toInt());
}

void MainWindow::showTopicInStudyPlan(int topic)
{
    for (int i = 0; i < studyList->count(); i++) {
        if (studyList->item(i)->data(Qt::UserRole).toInt() == topic) {
            studyList->setCurrentRow(i);
            break;
        }
    }
    tabs->setCurrentWidget(studyTab);
}

void MainWindow::refreshGraph()
{
    QVector<GraphView::Node> nodes;
    for (int t = 0; t < engine.topicCount(); t++)
        nodes.append({t, engine.topicName(t), engine.topic(t).priority});
    graphView->setGraph(nodes, engine.prerequisiteEdges(), engine.studyOrder());
}



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

    refreshGraph();
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
        graphView->setSelection(-1, {}, {});
        overviewChart->setHighlighted(-1);
        return;
    }
    const int topic = item->data(Qt::UserRole).toInt();
    studyTitle->setText(engine.topicName(topic));

    QSet<int> before, after;
    for (const auto &pair : engine.prerequisitesOf(topic)) before.insert(pair.first);
    for (const auto &pair : engine.dependentsOf(topic)) after.insert(pair.first);
    graphView->setSelection(topic, before, after);
    overviewChart->setHighlighted(topic);

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
