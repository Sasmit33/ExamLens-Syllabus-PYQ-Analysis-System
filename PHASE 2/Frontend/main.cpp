#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Exam Question Analyzer");

    MainWindow window;
    window.resize(1200, 760);
    window.show();
    return app.exec();
}
