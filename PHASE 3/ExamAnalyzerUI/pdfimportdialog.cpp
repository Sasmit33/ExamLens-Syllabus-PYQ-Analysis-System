#include "pdfimportdialog.h"
#include "pdfconverter.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QVBoxLayout>

PdfImportDialog::PdfImportDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Import PDFs");
    resize(680, 420);

    auto *v = new QVBoxLayout(this);
    auto *hint = new QLabel("Pick the syllabus PDF and the question paper PDFs. "
                            "They are converted to text and loaded into the analyzer.");
    hint->setWordWrap(true);
    v->addWidget(hint);

    // Syllabus row
    auto *top = new QGridLayout;
    syllabusEdit = new QLineEdit;
    syllabusEdit->setPlaceholderText("syllabus.pdf");
    auto *browseSyl = new QPushButton("Browse");
    connect(browseSyl, &QPushButton::clicked, this, &PdfImportDialog::browseSyllabus);
    top->addWidget(new QLabel("Syllabus PDF"), 0, 0);
    top->addWidget(syllabusEdit, 0, 1);
    top->addWidget(browseSyl, 0, 2);
    top->setColumnStretch(1, 1);
    v->addLayout(top);

    // Question papers: file + year (year cell is editable)
    v->addWidget(new QLabel("Question papers (double-click a year to change it)"));
    paperTable = new QTableWidget(0, 2);
    paperTable->setHorizontalHeaderLabels({"PDF file", "Year"});
    paperTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    paperTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    paperTable->verticalHeader()->setVisible(false);
    paperTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    v->addWidget(paperTable, 1);

    auto *paperButtons = new QHBoxLayout;
    auto *add = new QPushButton("Add papers...");
    auto *remove = new QPushButton("Remove selected");
    connect(add, &QPushButton::clicked, this, &PdfImportDialog::browsePapers);
    connect(remove, &QPushButton::clicked, this, &PdfImportDialog::removeSelectedPaper);
    paperButtons->addWidget(add);
    paperButtons->addWidget(remove);
    paperButtons->addStretch();
    v->addLayout(paperButtons);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    QPushButton *go = buttons->addButton("Convert and analyze", QDialogButtonBox::AcceptRole);
    go->setDefault(true);
    connect(go, &QPushButton::clicked, this, &PdfImportDialog::convert);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    v->addWidget(buttons);
}

// Find a year like 2023 in "paper_2023.pdf" or "DS-Dec-2024.pdf"; 0 if none.
int PdfImportDialog::guessYear(const QString &fileName)
{
    static const QRegularExpression re("(?:^|\\D)((?:19|20)\\d\\d)(?:\\D|$)");
    const QRegularExpressionMatch m = re.match(fileName);
    return m.hasMatch() ? m.captured(1).toInt() : 0;
}

void PdfImportDialog::setSyllabusPdf(const QString &path)
{
    syllabusEdit->setText(QDir::toNativeSeparators(path));
}

void PdfImportDialog::addPaperPdfs(const QStringList &paths)
{
    for (const QString &path : paths) {
        const int row = paperTable->rowCount();
        paperTable->insertRow(row);

        auto *file = new QTableWidgetItem(QFileInfo(path).fileName());
        file->setData(Qt::UserRole, QDir::toNativeSeparators(path));   // full path kept hidden
        file->setToolTip(QDir::toNativeSeparators(path));
        file->setFlags(file->flags() & ~Qt::ItemIsEditable);

        const int year = guessYear(QFileInfo(path).fileName());
        auto *yearItem = new QTableWidgetItem(year ? QString::number(year) : QString());

        paperTable->setItem(row, 0, file);
        paperTable->setItem(row, 1, yearItem);
    }
}

void PdfImportDialog::browseSyllabus()
{
    const QString path = QFileDialog::getOpenFileName(this, "Select syllabus PDF", QDir::homePath(),
                                                      "PDF files (*.pdf)");
    if (!path.isEmpty()) setSyllabusPdf(path);
}

void PdfImportDialog::browsePapers()
{
    const QStringList paths = QFileDialog::getOpenFileNames(this, "Select question paper PDFs",
                                                            QDir::homePath(), "PDF files (*.pdf)");
    addPaperPdfs(paths);
}

void PdfImportDialog::removeSelectedPaper()
{
    const int row = paperTable->currentRow();
    if (row >= 0) paperTable->removeRow(row);
}

void PdfImportDialog::convert()
{
    // 1. Check the inputs before starting Python
    const QString syllabus = syllabusEdit->text().trimmed();
    if (syllabus.isEmpty() || !QFileInfo::exists(syllabus)) {
        QMessageBox::warning(this, "Syllabus missing", "Choose the syllabus PDF.");
        return;
    }
    if (paperTable->rowCount() == 0) {
        QMessageBox::warning(this, "No question papers", "Add at least one question paper PDF.");
        return;
    }

    QStringList papers;
    QList<int> years;
    for (int r = 0; r < paperTable->rowCount(); r++) {
        const QString name = paperTable->item(r, 0)->text();
        bool ok = false;
        const int year = paperTable->item(r, 1)->text().trimmed().toInt(&ok);
        if (!ok || year < 1900 || year > 2100) {
            QMessageBox::warning(this, "Year missing",
                                 QString("Enter the exam year for %1 (e.g. 2024).").arg(name));
            paperTable->setCurrentCell(r, 1);
            return;
        }
        papers << paperTable->item(r, 0)->data(Qt::UserRole).toString();
        years << year;
    }

    // 2. Run the parser twice: syllabus, then all papers together
    PdfConverter converter;
    QString error;
    if (!converter.isReady(&error)) {
        QMessageBox::critical(this, "PDF parser not available", error);
        return;
    }

    const QDir out(PdfConverter::outputFolder());
    const QString sylTxt = QDir::toNativeSeparators(out.filePath("syllabus.txt"));
    const QString qTxt = QDir::toNativeSeparators(out.filePath("questions.txt"));

    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = converter.convertSyllabus(syllabus, sylTxt, &error);
    if (!ok) error = "Syllabus PDF: " + error;
    if (ok) {
        ok = converter.convertQuestions(papers, years, qTxt, &error);
        if (!ok) error = "Question papers: " + error;
    }
    QApplication::restoreOverrideCursor();

    if (!ok) {
        QMessageBox::critical(this, "Conversion failed", error);
        return;
    }

    syllabusOut = sylTxt;
    questionsOut = qTxt;
    accept();
}
