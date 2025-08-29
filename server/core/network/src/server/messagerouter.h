#pragma once

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QHash>
#include <protocol.h>

class ClientHandler;

// 消息路由器（单例）：
// - 广播 JSON 请求（payload 内自带 uuid）给业务层
// - 接收业务层响应（payload 内自带 request_uuid）并路由回对应的 ClientHandler
class MessageRouter : public QObject {
    Q_OBJECT
public:
    static MessageRouter& instance();

public slots:
    // 来自每个连接线程的原始请求（由 CommunicationServer 连接）
    void onRequestReady(ClientHandler* sender, Protocol::Header header, QJsonObject payload);

    // 业务层回馈响应：payload 内需包含 request_uuid，用于路由回对应连接
    void onBusinessResponse(Protocol::MessageType type,
                            QJsonObject payload);

signals:
    // 向指定的 ClientHandler 返回响应（由路由器发射，具体发送在对应 handler 线程执行）
    void responseReady(ClientHandler* target,
                       Protocol::MessageType type,
                       QJsonObject payload);

    // 向业务层广播一条 JSON 请求（payload 内含 uuid 字段）
    void requestReceived(QJsonObject payload);

private:
    explicit MessageRouter(QObject* parent = nullptr);
    void handleJson(ClientHandler* sender, const Protocol::Header& header, QJsonObject payload);

    // 记录 uuid -> ClientHandler，用于响应路由
    QHash<QString, QPointer<ClientHandler>> m_uuidToHandler;
    // 清理所有属于某个 handler 的未完成路由（当其销毁时）
    void cleanupRoutesFor(ClientHandler* handler);
};