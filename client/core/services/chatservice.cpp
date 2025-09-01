#include "core/services/chatservice.h"
#include "core/logging/logging.h"
#include "core/network/communicationclient.h"
#include <QTimer>
#include <QUuid>
#include <algorithm>

ChatService::ChatService(CommunicationClient* client, const QString& currentUser, QObject* parent)
    : QObject(parent)
    , m_client(client)
    , m_currentUser(currentUser)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &ChatService::onJsonReceived);
}

void ChatService::requestChat(const QString& doctorUser, const QString& patientUser, const QString& note)
{
    QJsonObject req { { "action", "request_chat" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "user", m_currentUser }, { "doctor_user", doctorUser }, { "patient_user", patientUser }, { "note", note } };
    Log::request("ChatService", req, "action", "request_chat");
    m_client->sendJson(req);
}

void ChatService::acceptChat(const QString& doctorUser, const QString& patientUser)
{
    QJsonObject req { { "action", "accept_chat" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "user", m_currentUser }, { "doctor_user", doctorUser }, { "patient_user", patientUser } };
    Log::request("ChatService", req, "action", "accept_chat");
    m_client->sendJson(req);
}

void ChatService::sendText(const QString& doctorUser, const QString& patientUser, const QString& text)
{
    QJsonObject req { { "action", "send_message" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "user", m_currentUser }, { "doctor_user", doctorUser }, { "patient_user", patientUser },
        { "message_id", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "message_type", "text" }, { "text_content", text }, { "file_metadata", QJsonValue() } };
    Log::request("ChatService", req, "action", "send_message");
    m_client->sendJson(req);
}

void ChatService::getHistory(const QString& doctorUser, const QString& patientUser, qint64 beforeId, int limit)
{
    QJsonObject req { { "action", "get_history_messages" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "doctor_user", doctorUser }, { "patient_user", patientUser }, { "before_id", (double)beforeId }, { "limit", limit } };
    Log::request("ChatService", req, "action", "get_history_messages");
    m_client->sendJson(req);
}

void ChatService::pollEvents(qint64 cursor, int timeoutSec, int limit)
{
    QJsonObject req { { "action", "poll_events" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "user", m_currentUser }, { "cursor", (double)cursor }, { "timeout_sec", timeoutSec }, { "limit", limit } };
    Log::request("ChatService", req, "action", "poll_events");
    m_client->sendJson(req);
}

void ChatService::recentContacts(int limit)
{
    QJsonObject req { { "action", "recent_contacts" }, { "uuid", QUuid::createUuid().toString(QUuid::WithoutBraces) },
        { "user", m_currentUser }, { "limit", limit } };
    Log::request("ChatService", req, "action", "recent_contacts");
    m_client->sendJson(req);
}

void ChatService::onJsonReceived(const QJsonObject& obj)
{
    const QString type = obj.value("type").toString();
    if (type == "request_chat_response") {
        emit requestChatResult(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "accept_chat_response") {
        emit acceptChatResult(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "send_message_response") {
        emit sendMessageResult(obj.value("success").toBool(), obj.value("data").toObject());
        return;
    }
    if (type == "get_history_messages_response") {
        QJsonObject data = obj.value("data").toObject();
        const auto arr = data.value("messages").toArray();
        const bool hasMore = data.value("has_more").toBool();
        emit historyReceived(arr, hasMore);

        // 管理器：合并到缓存（服务端返回 id DESC，需要倒序转为 ASC）
        QString doctor = data.value("doctor_user").toString();
        QString patient = data.value("patient_user").toString();
        QList<QJsonObject> pageAsc;
        pageAsc.reserve(arr.size());
        for (int i = arr.size() - 1; i >= 0; --i)
            pageAsc.push_back(arr.at(i).toObject());
        if ((doctor.isEmpty() || patient.isEmpty()) && !pageAsc.isEmpty()) {
            const auto& o0 = pageAsc.back(); // 最新一条
            if (doctor.isEmpty())
                doctor = o0.value("doctor_username").toString();
            if (patient.isEmpty())
                patient = o0.value("patient_username").toString();
        }
        const QString key = convKey(doctor, patient);
        QList<QJsonObject> delta;
        mergeAscending(key, pageAsc, &delta);
        // 维护 earliestId
        if (!pageAsc.isEmpty()) {
            const qint64 earliest = pageAsc.first().value("id").toVariant().toLongLong();
            const auto it = m_earliestId.find(key);
            if (it == m_earliestId.end() || earliest < it.value())
                m_earliestId[key] = earliest;
        }
        emit conversationHistoryLoaded(doctor, patient, pageAsc, hasMore, m_earliestId.value(key, 0));
        return;
    }
    if (type == "poll_events_response") {
        QJsonObject data = obj.value("data").toObject();
        const auto messages = data.value("messages").toArray();
        const auto instant = data.value("instant_events").toArray();
        const qint64 nextCursor = (qint64)data.value("next_cursor").toDouble();
        const bool hasMore = data.value("has_more").toBool();
        emit eventsReceived(messages, instant, nextCursor, hasMore);

        // 更新游标并串联下一次轮询（由 Service 控制）
        m_pollCursor = nextCursor;
        m_pollInFlight = false;
        if (m_polling) {
            const bool gotSomething = (!messages.isEmpty() || !instant.isEmpty() || hasMore);
            // 若为空包（可能是另一端设备已占用挂起，服务端对本端立即返回空），使用轻微回退避免忙轮询
            scheduleNextPoll(gotSomething ? 0 : 1000);
        }

        // 将 events 中的消息增量并入缓存（通常是最新的消息，可能跨多个会话）
        // 这些消息服务端通常已是按时间升序或未知顺序，这里保持按 id 排序再合并
        QMap<QString, QList<QJsonObject>> batchByConv;
        for (const auto& v : messages) {
            const QJsonObject m = v.toObject();
            const QString doctor = m.value("doctor_username").toString();
            const QString patient = m.value("patient_username").toString();
            batchByConv[convKey(doctor, patient)].push_back(m);
        }
        for (auto it = batchByConv.begin(); it != batchByConv.end(); ++it) {
            auto& lst = it.value();
            std::sort(lst.begin(), lst.end(), [](const QJsonObject& a, const QJsonObject& b) {
                return a.value("id").toVariant().toLongLong() < b.value("id").toVariant().toLongLong();
            });
            QList<QJsonObject> delta;
            mergeAscending(it.key(), lst, &delta);
            // 维护 earliestId
            if (!lst.isEmpty()) {
                const qint64 earliest = lst.first().value("id").toVariant().toLongLong();
                const auto eit = m_earliestId.find(it.key());
                if (eit == m_earliestId.end() || earliest < eit.value())
                    m_earliestId[it.key()] = earliest;
            }
            // 解析 key -> doctor/patient 供信号携带
            const auto parts = it.key().split('|');
            const QString doctor = parts.value(0);
            const QString patient = parts.value(1);
            emit conversationUpserted(doctor, patient, delta);
        }
        return;
    }
    if (type == "recent_contacts_response") {
        QJsonObject data = obj.value("data").toObject();
        emit recentContactsReceived(data.value("contacts").toArray());
        return;
    }
}

void ChatService::scheduleNextPoll(int delayMs)
{
    if (!m_polling || m_pollInFlight)
        return;
    m_pollInFlight = true;
    QTimer::singleShot(delayMs, this, [this]() { doPoll(); });
}

void ChatService::doPoll()
{
    // 使用较大的超时与默认 limit
    pollEvents(m_pollCursor);
}

void ChatService::startPolling()
{
    if (m_polling)
        return;
    m_polling = true;
    scheduleNextPoll();
}

void ChatService::stopPolling()
{
    m_polling = false;
}

QString ChatService::convKey(const QString& doctorUser, const QString& patientUser)
{
    return doctorUser + '|' + patientUser;
}

void ChatService::mergeAscending(const QString& key, const QList<QJsonObject>& pageAsc, QList<QJsonObject>* deltaOut)
{
    auto& store = m_convMessages[key];
    QList<QJsonObject> added;
    // 归并：按 id 升序加入，去重
    int i = 0, j = 0;
    QList<QJsonObject> merged;
    merged.reserve(store.size() + pageAsc.size());
    auto idOf = [](const QJsonObject& o) { return o.value("id").toVariant().toLongLong(); };
    // 先将现有与新页合并（两者均升序）
    // 为简单起见，这里先拼接再排序去重，避免复杂双指针（消息量有限）
    merged = store;
    merged.append(pageAsc);
    std::sort(merged.begin(), merged.end(), [&](const QJsonObject& a, const QJsonObject& b) { return idOf(a) < idOf(b); });
    // 去重并判断增量
    QList<qint64> seen;
    seen.reserve(merged.size());
    QList<QJsonObject> unique;
    unique.reserve(merged.size());
    for (const auto& m : merged) {
        const qint64 id = idOf(m);
        if (!seen.isEmpty() && seen.back() == id)
            continue;
        // 判断是否为新增（不在原 store 中）
        bool exists = false;
        for (const auto& old : store) {
            if (idOf(old) == id) {
                exists = true;
                break;
            }
        }
        if (!exists)
            added.push_back(m);
        unique.push_back(m);
        seen.push_back(id);
    }
    store.swap(unique);
    if (deltaOut)
        *deltaOut = added;
}

QList<QJsonObject> ChatService::messagesFor(const QString& doctorUser, const QString& patientUser) const
{
    const auto key = convKey(doctorUser, patientUser);
    return m_convMessages.value(key);
}

qint64 ChatService::earliestIdFor(const QString& doctorUser, const QString& patientUser) const
{
    return m_earliestId.value(convKey(doctorUser, patientUser), 0);
}
