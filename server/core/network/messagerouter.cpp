#include "core/network/clienthandler.h"
#include "core/network/messagerouter.h"
#include <QDateTime>
#include <QUuid>

using namespace Protocol;

MessageRouter& MessageRouter::instance()
{
    static MessageRouter inst;
    return inst;
}

MessageRouter::MessageRouter(QObject* parent)
    : QObject(parent)
{
    // 注册用于跨线程队列连接的元类型
    qRegisterMetaType<Protocol::MessageType>("Protocol::MessageType");
    qRegisterMetaType<Protocol::Header>("Protocol::Header");
}

void MessageRouter::onRequestReady(ClientHandler* sender, Header header, QJsonObject payload)
{
    // qInfo() << "[Dispatcher] 收到请求，type=" << (quint16)header.type
    //         << ", keys=" << payload.keys().size();
    switch (header.type) {
    case MessageType::JsonRequest:
        qInfo() << "[Router] 广播JSON请求给业务层";
        handleJson(sender, header, payload);
        break;
    case MessageType::ClientDisconnect: {
        // 客户端断开：清理该连接相关的路由映射
        cleanupRoutesFor(sender);
        qInfo() << "[Router] 已清理断开连接的路由";
        break;
    }
    case MessageType::HeartbeatPing: {
        // 心跳无需 JSON 载荷
        emit responseReady(sender, MessageType::HeartbeatPong, QJsonObject());
        // qInfo() << "[Router] 已回复心跳PONG";
        break;
    }
    default: {
        QJsonObject err { { "errorCode", 400 }, { "errorMessage", QStringLiteral("Unsupported message type") } };
        emit responseReady(sender, MessageType::ErrorResponse, err);
        qWarning() << "[Router] 未支持的消息类型，已回复错误";
        break;
    }
    }
}

void MessageRouter::handleJson(ClientHandler* sender, const Header& header, QJsonObject payload)
{
    // 1) 确保存在 uuid 字段；如无则创建
    QString uuid = payload.value("uuid").toString();
    if (uuid.isEmpty()) {
        uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
        payload.insert("uuid", uuid);
    }

    // 2) 记录路由关系：uuid -> sender（弱引用）
    m_uuidToHandler.insert(uuid, QPointer<ClientHandler>(sender));

    // 3) 广播给业务层
    qInfo() << "[Router] 广播业务请求 uuid=" << uuid;
    emit requestReceived(payload);
}

void MessageRouter::onBusinessResponse(QJsonObject payload)
{
    // 从响应 payload 中读取 request_uuid
    const QString uuid = payload.value("request_uuid").toString();
    if (uuid.isEmpty()) {
        qWarning() << "[Router] 响应缺少 request_uuid 字段，已丢弃";
        return;
    }
    auto it = m_uuidToHandler.find(uuid);
    if (it == m_uuidToHandler.end()) {
        qWarning() << "[Router] 未找到 uuid 的目标连接，丢弃响应 uuid=" << uuid;
        return;
    }
    QPointer<ClientHandler> target = it.value();
    m_uuidToHandler.erase(it);
    if (!target) {
        qWarning() << "[Router] 目标连接已失效，丢弃响应 uuid=" << uuid;
        return;
    }
    // 统一为 JSON 响应类型
    qInfo() << "[Router] 路由响应给目标连接 uuid=" << uuid;
    emit responseReady(target, MessageType::JsonResponse, payload);
}

void MessageRouter::cleanupRoutesFor(ClientHandler* handler)
{
    // 移除所有映射到该 handler 的 uuid
    QList<QString> toRemove;
    for (auto it = m_uuidToHandler.constBegin(); it != m_uuidToHandler.constEnd(); ++it) {
        if (it.value() == handler) toRemove.append(it.key());
    }
    for (const auto& k : toRemove) m_uuidToHandler.remove(k);
}
