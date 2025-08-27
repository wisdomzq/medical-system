#include "hello.h"

#include <QApplication>
#pragma comment(lib, "user32.lib")

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Hello w;
    w.setWindowTitle("智慧医疗系统");
    w.show();
    return a.exec();
}