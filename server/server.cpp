#include "tcpserver.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TcpServer server;
    if (server.listen(QHostAddress::Any, 12345)) {
        qDebug() << "Server started on port 12345...";
    } else {
        qDebug() << "Server failed to start.";
    }

    return a.exec();
}
