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
    // 注册用于跨线程队列连接的元类型（仅需 MessageType 给 responseReady 使用）
    qRegisterMetaType<Protocol::MessageType>("Protocol::MessageType");
}

void MessageRouter::onJsonRequest(ClientHandler* sender, QJsonObject payload)
{
    Q_UNUSED(sender);
    qInfo() << "[ Router ] 广播JSON请求给业务层";
    handleJson(sender, payload);
}

void MessageRouter::handleJson(ClientHandler* sender, QJsonObject payload)
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
    qInfo() << "[ Router ] 广播业务请求 uuid=" << uuid;
    emit requestReceived(payload);
}

void MessageRouter::onBusinessResponse(QJsonObject payload)
{
    // 从响应 payload 中读取 request_uuid
    const QString uuid = payload.value("request_uuid").toString();
    if (uuid.isEmpty()) {
        qWarning() << "[ Router ] 响应缺少 request_uuid 字段，已丢弃";
        return;
    }
    auto it = m_uuidToHandler.find(uuid);
    if (it == m_uuidToHandler.end()) {
        qWarning() << "[ Router ] 未找到 uuid 的目标连接，丢弃响应 uuid=" << uuid;
        return;
    }
    QPointer<ClientHandler> target = it.value();
    m_uuidToHandler.erase(it);
    if (!target) {
        qWarning() << "[ Router ] 目标连接已失效，丢弃响应 uuid=" << uuid;
        return;
    }
    // 统一为 JSON 响应类型
    qInfo() << "[ Router ] 路由响应给目标连接 uuid=" << uuid;
    emit responseReady(target, payload);
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

void MessageRouter::onClientHandlerDestroyed(QObject* obj)
{
    auto* handler = qobject_cast<ClientHandler*>(obj);
    if (!handler) return;
    cleanupRoutesFor(handler);
    qInfo() << "[ Router ] 处理 handler 销毁清理完成";
}
