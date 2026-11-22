#pragma once

#include <QList>
#include <QString>
#include <QStringList>

// Runs the Python PDF parser (exam_analyzer/tools/pdf_convert.py) with QProcess.
// Python only turns PDF into text. All analysis stays in the C engine.
class PdfConverter
{
public:
    PdfConverter();

    bool isReady(QString *error) const;   // Python and script both found?

    bool convertSyllabus(const QString &pdf, const QString &outTxt, QString *error) const;
    bool convertQuestions(const QStringList &pdfs, const QList<int> &years,
                          const QString &outTxt, QString *error) const;

    static QString outputFolder();        // where the converted .txt files go

private:
    bool run(const QStringList &args, QString *error) const;
    static QString findScript();
    static QString findPython();

    QString script;
    QString python;
};
