#include "hello.h"

#include <QApplication>
#include <QFile>
#include <QTextStream>
#pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Load and apply the stylesheet
    QFile file(":/style.qss");
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        a.setStyleSheet(stream.readAll());
        file.close();
    }

    Hello w;
    w.setWindowTitle("智慧医疗系统");
    w.show();
    return a.exec();
}