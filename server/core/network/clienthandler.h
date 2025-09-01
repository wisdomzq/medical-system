#pragma once

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>

#include "core/network/protocol.h"
class FileTransferProcessor;

// 每个客户端连接对应一个 ClientHandler，在独立线程中解析协议并通过信号交给路由器/业务层处理
class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(QObject* parent = nullptr);
    ~ClientHandler();

    void initialize(qintptr socketDescriptor);

signals:
    // 仅向路由层发送 JSON 请求（已过滤非 JSON 类型的数据包）
    void requestJsonReady(ClientHandler* sender, QJsonObject payload);

public slots:
    void onJsonResponseReady(const QJsonObject& obj);
    // 发送不同类型消息的便捷重载
    void sendMessage(Protocol::MessageType type, const QJsonObject& obj);
    void sendMessage(Protocol::MessageType type);                 // 空payload
    void sendBinary(Protocol::MessageType type, const QByteArray& data);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket = nullptr;
    // 解析器改为与客户端一致的 StreamFrameParser
    class StreamFrameParser* m_parser = nullptr;
    Protocol::Header m_currentHeader;
    FileTransferProcessor* m_file = nullptr;

    // 解析已移入 m_parser
};
