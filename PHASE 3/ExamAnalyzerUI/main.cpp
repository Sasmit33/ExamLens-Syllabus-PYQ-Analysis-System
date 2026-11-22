#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Exam Question Analyzer");
    QApplication::setStyle("Fusion");     

    MainWindow window;
    window.resize(1280, 820);
    window.show();
    return app.exec();
}
