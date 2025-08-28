#include "tcpserver.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    TcpServer server;
    server.listen(QHostAddress::Any, 12345);

    return a.exec();
}
