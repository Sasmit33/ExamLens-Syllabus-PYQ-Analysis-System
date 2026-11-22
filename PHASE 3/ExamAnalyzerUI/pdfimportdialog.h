#pragma once

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QTableWidget;

// "Import PDFs" window: one syllabus PDF + question paper PDFs (each with a year).
// On Convert it runs PdfConverter and keeps the two .txt paths for MainWindow.
class PdfImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PdfImportDialog(QWidget *parent = nullptr);

    void setSyllabusPdf(const QString &path);
    void addPaperPdfs(const QStringList &paths);

    QString syllabusTxt() const { return syllabusOut; }
    QString questionsTxt() const { return questionsOut; }

private slots:
    void browseSyllabus();
    void browsePapers();
    void removeSelectedPaper();
    void convert();

private:
    static int guessYear(const QString &fileName);

    QLineEdit *syllabusEdit = nullptr;
    QTableWidget *paperTable = nullptr;
    QString syllabusOut;
    QString questionsOut;
};
