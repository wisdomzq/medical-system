#pragma once

#include <QDebug>
#include <QTcpServer>

class ClientHandler;

// 服务端监听器：负责接收新连接并将处理工作委派到 ClientHandler（独立线程）
class CommunicationServer : public QTcpServer {
    Q_OBJECT
public:
    explicit CommunicationServer(QObject* parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
};
