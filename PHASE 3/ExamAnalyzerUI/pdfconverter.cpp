#include "pdfconverter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>

PdfConverter::PdfConverter() : script(findScript()), python(findPython()) {}

// Look for tools/pdf_convert.py: first the path CMake recorded at build time,
// then walk up from the app's folder (same idea as MainWindow::findDefault).
QString PdfConverter::findScript()
{
#ifdef EXAMLENS_TOOLS_DIR
    const QString built = QDir(QStringLiteral(EXAMLENS_TOOLS_DIR)).filePath("pdf_convert.py");
    if (QFileInfo::exists(built)) return QDir::cleanPath(built);
#endif
    QDir dir(QCoreApplication::applicationDirPath());
    for (int depth = 0; depth < 6; depth++) {
        for (const QString &rel : {QString("exam_analyzer/tools/pdf_convert.py"),
                                   QString("tools/pdf_convert.py")}) {
            const QString candidate = dir.filePath(rel);
            if (QFileInfo::exists(candidate)) return QDir::cleanPath(candidate);
        }
        if (!dir.cdUp()) break;
    }
    return QString();
}

// Pick a Python that has pdfplumber installed.
// Order: EXAMLENS_PYTHON variable, the ~/.venvs/examlens venv, then python3/python/py.
// Fixed Homebrew paths are listed because apps opened from Finder get a short PATH.
QString PdfConverter::findPython()
{
    QStringList candidates;
    const QString fromEnv = qEnvironmentVariable("EXAMLENS_PYTHON");
    if (!fromEnv.isEmpty()) candidates << fromEnv;

    const QString venv = QDir::home().filePath(".venvs/examlens");
    candidates << venv + "/bin/python3" << venv + "/bin/python" << venv + "/Scripts/python.exe";

    for (const QString &name : {QString("python3"), QString("python"), QString("py")}) {
        const QString found = QStandardPaths::findExecutable(name);
        if (!found.isEmpty()) candidates << found;
    }
    candidates << "/opt/homebrew/bin/python3" << "/usr/local/bin/python3" << "/usr/bin/python3";

    QString firstWorking;
    for (const QString &exe : candidates) {
        if (!QFileInfo(exe).isExecutable()) continue;
        if (firstWorking.isEmpty()) firstWorking = exe;

        QProcess probe;
        probe.start(exe, {"-c", "import pdfplumber"});
        if (probe.waitForFinished(15000) && probe.exitStatus() == QProcess::NormalExit
            && probe.exitCode() == 0)
            return exe;
    }
    return firstWorking;   // no pdfplumber anywhere: the script will say how to install it
}

QString PdfConverter::outputFolder()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) base = QDir::temp().filePath("ExamAnalyzer");
    const QString folder = QDir(base).filePath("converted");
    QDir().mkpath(folder);
    return folder;
}

bool PdfConverter::isReady(QString *error) const
{
    if (script.isEmpty()) {
        if (error) *error = "Could not find exam_analyzer/tools/pdf_convert.py.\n"
                            "Keep ExamAnalyzerUI and exam_analyzer in the same parent folder.";
        return false;
    }
    if (python.isEmpty()) {
        if (error) *error = "Python 3 was not found.\n"
                            "Install Python 3, then run: pip install pdfplumber";
        return false;
    }
    return true;
}

bool PdfConverter::run(const QStringList &args, QString *error) const
{
    if (!isReady(error)) return false;

    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");   // Windows consoles default to another code page
    p.setProcessEnvironment(env);
    p.start(python, QStringList{script} + args);

    if (!p.waitForStarted(10000)) {
        if (error) *error = "Could not start Python: " + python;
        return false;
    }
    if (!p.waitForFinished(120000)) {
        p.kill();
        if (error) *error = "The PDF parser took too long (over 2 minutes).";
        return false;
    }
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        // pdf_convert.py prints a one-line reason on stderr, e.g. scanned PDF, wrong year count.
        QString msg = QString::fromUtf8(p.readAllStandardError()).trimmed();
        if (msg.isEmpty()) msg = "The PDF parser failed (exit code " + QString::number(p.exitCode()) + ").";
        if (error) *error = msg;
        return false;
    }
    return true;
}

bool PdfConverter::convertSyllabus(const QString &pdf, const QString &outTxt, QString *error) const
{
    return run({"--type", "syllabus", pdf, "-o", outTxt}, error);
}

bool PdfConverter::convertQuestions(const QStringList &pdfs, const QList<int> &years,
                                    const QString &outTxt, QString *error) const
{
    QStringList yearText;
    for (int y : years) yearText << QString::number(y);
    return run(QStringList{"--type", "questions", "--year", yearText.join(',')} + pdfs + QStringList{"-o", outTxt},
               error);
}
