#include "tcpserver.h"
#include "database/database.h"
#include "database/dbterminal.h"
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 创建数据库管理器
    DBManager dbManager("user.db");
   
    // 在这里添加数据库状态检查（在创建terminal之前）
    qDebug() << "数据库连接状态:" << dbManager.getDatabase().isOpen();
    qDebug() << "数据库驱动:" << dbManager.getDatabase().driverName();
 
    // 创建数据库终端监控
    DBTerminal terminal(&dbManager);
    terminal.startMonitoring(10000); // 每10秒更新一次
    
    // 启动TCP服务器
    TcpServer server;
    if (server.listen(QHostAddress::Any, 12345)) {
        qDebug() << "Server started on port 12345...";
    } else {
        qDebug() << "Server failed to start.";
    }

    return a.exec();
}