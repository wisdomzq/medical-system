#pragma once

#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <protocol.h>

class StreamFrameParser;
class ResponseDispatcher;

// 客户端通信封装：
// - 负责与服务器建立 TCP 连接
// - 进行认证、心跳维持
// - 发送/接收 JSON 请求与响应
class CommunicationClient : public QObject {
    Q_OBJECT
public:
    explicit CommunicationClient(QObject* parent = nullptr);

    void connectToServer(const QString& host, quint16 port);

signals:
    void connected();
    void disconnected();
    void jsonReceived(const QJsonObject& obj);
    void errorOccurred(int code, const QString& message);

public slots:
    void sendJson(const QJsonObject& obj);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void sendHeartbeat();
    void onHeartbeatTimeout();

private:
    // TCP 套接字（直接成员，随对象生命周期自动析构）
    QTcpSocket m_socket;
    // 解析与分发
    StreamFrameParser* m_parser = nullptr;
    ResponseDispatcher* m_dispatcher = nullptr;
    QTimer m_pingTimer;
    QTimer m_pongTimeoutTimer;
    int m_reconnectDelay = 1000; // ms
    QString m_host;
    quint16 m_port = 0;
};
