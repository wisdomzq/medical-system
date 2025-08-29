#pragma once

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QTcpSocket>

#include <protocol.h>

// 每个客户端连接对应一个 ClientHandler，在独立线程中解析协议并通过信号交给调度器处理
class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(QObject* parent = nullptr);
    ~ClientHandler();

    void initialize(qintptr socketDescriptor);

signals:
    void requestReady(ClientHandler* sender, Protocol::Header header, QJsonObject payload);

public slots:
    void sendMessage(Protocol::MessageType type, const QJsonObject& obj);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    // 简易状态机：按顺序读取头、payload
    enum class ParseState {
        WAITING_FOR_HEADER,
        WAITING_FOR_PAYLOAD
    };

    QTcpSocket* m_socket = nullptr;
    QByteArray m_buffer;
    ParseState m_state = ParseState::WAITING_FOR_HEADER;

    Protocol::Header m_currentHeader;

    bool ensureBytesAvailable(int count) const { return m_buffer.size() >= count; }
    bool parseFixedHeader(Protocol::Header& out, int& headerBytesConsumed);
};
