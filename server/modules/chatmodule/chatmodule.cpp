#include "modules/chatmodule/chatmodule.h"
#include "core/network/messagerouter.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>

ChatModule::ChatModule(QObject *parent):QObject(parent), m_db(new DBManager(DatabaseConfig::getDatabasePath())) {
    // 订阅总线
    QObject::connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
                     this, &ChatModule::onRequest);
    QObject::connect(this, &ChatModule::businessResponse,
                     &MessageRouter::instance(), &MessageRouter::onBusinessResponse);
}

ChatModule::~ChatModule(){ delete m_db; }

void ChatModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("ChatModule", resp);
    emit businessResponse(resp);
}

void ChatModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    if (a != "request_chat" && a != "accept_chat" && a != "send_message"
        && a != "get_history_messages" && a != "poll_events" && a != "recent_contacts") return; // 非本模块
    Log::request("ChatModule", payload, "action", a);
    QJsonObject resp;
    if (a == "request_chat") resp = handleRequestChat(payload);
    else if (a == "accept_chat") resp = handleAcceptChat(payload);
    else if (a == "send_message") resp = handleSendMessage(payload);
    else if (a == "get_history_messages") resp = handleGetHistory(payload);
    else if (a == "poll_events") resp = handlePollEvents(payload);
    else if (a == "recent_contacts") resp = handleRecentContacts(payload);
    // handlePollEvents 可能异步应答（长轮询），若返回了空对象表示已挂起，不要立即reply
    if (!resp.isEmpty()) reply(resp, payload);
}

QJsonObject ChatModule::handleRequestChat(const QJsonObject &req) {
    const QString doctor = req.value("doctor_user").toString();
    const QString patient = req.value("patient_user").toString();
    const QString note = req.value("note").toString();
    QJsonObject event{{"event_type","chat_requested"},{"doctor_user",doctor},{"patient_user",patient},{"note",note}};
    enqueueInstantForPair(doctor, patient, event);
    return QJsonObject{{"type","request_chat_response"},{"success",true},{"message",""},{"data",QJsonObject{{"status","pending"}}}};
}

QJsonObject ChatModule::handleAcceptChat(const QJsonObject &req) {
    const QString doctor = req.value("doctor_user").toString();
    const QString patient = req.value("patient_user").toString();
    QJsonObject event{{"event_type","chat_accepted"},{"doctor_user",doctor},{"patient_user",patient}};
    enqueueInstantForPair(doctor, patient, event);
    return QJsonObject{{"type","accept_chat_response"},{"success",true},{"data",QJsonObject{{"status","active"}}}};
}

QJsonObject ChatModule::handleSendMessage(const QJsonObject &req) {
    // 将消息写入数据库
    int id = 0; QString err;
    QJsonObject toInsert{
        {"doctor_username", req.value("doctor_user").toString()},
        {"patient_username", req.value("patient_user").toString()},
        {"message_id", req.value("message_id").toString()},
        {"sender_username", req.value("user").toString()},
        {"message_type", req.value("message_type").toString()},
        {"text_content", req.value("text_content").toString()}
    };
    if (req.contains("file_metadata") && req.value("file_metadata").isObject())
        toInsert.insert("file_metadata", req.value("file_metadata").toObject());
    if (!m_db->addChatMessage(toInsert, id, err)) {
        return QJsonObject{{"type","send_message_response"},{"success",false},{"message",err}};
    }
    QJsonObject data{{"id", id},
                     {"doctor_user", req.value("doctor_user").toString()},
                     {"patient_user", req.value("patient_user").toString()},
                     {"message_id", req.value("message_id").toString()},
                     {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}};
    // 唤醒双方的挂起轮询
    const QString doctor = toInsert.value("doctor_username").toString();
    const QString patient = toInsert.value("patient_username").toString();
    fulfillPendingPoll(doctor);
    fulfillPendingPoll(patient);
    return QJsonObject{{"type","send_message_response"},{"success",true},{"data",data}};
}

QJsonObject ChatModule::handleGetHistory(const QJsonObject &req) {
    const QString doctor = req.value("doctor_user").toString();
    const QString patient = req.value("patient_user").toString();
    const qint64 beforeId = req.value("before_id").toVariant().toLongLong();
    const int limit = qMax(1, req.value("limit").toInt(20));
    QJsonArray arr;
    if (!m_db->getChatHistory(doctor, patient, beforeId, limit, arr)) {
        return QJsonObject{{"type","get_history_messages_response"},{"success",false},{"message","db error"}};
    }
    return QJsonObject{{"type","get_history_messages_response"},{"success",true},{"data", QJsonObject{{"doctor_user", doctor},{"patient_user", patient},{"messages", arr}, {"has_more", arr.size()>=limit}}}};
}

QJsonObject ChatModule::handleRecentContacts(const QJsonObject &req) {
    const QString user = req.value("user").toString();
    const int limit = qBound(1, req.value("limit").toInt(20), 100);
    QJsonArray arr;
    if (!m_db->getRecentContactsForUser(user, limit, arr)) {
        return QJsonObject{{"type","recent_contacts_response"},{"success",false},{"message","db error"}};
    }
    return QJsonObject{{"type","recent_contacts_response"},{"success",true},{"data", QJsonObject{{"contacts", arr}}}};
}

QJsonObject ChatModule::handlePollEvents(const QJsonObject &req) {
    const QString user = req.value("user").toString();
    const qint64 cursor = req.value("cursor").toVariant().toLongLong();
    const int timeoutSec = qBound(1, req.value("timeout_sec").toInt(1800), 1800); // 最长30分钟
    const int limit = qBound(1, req.value("limit").toInt(50), 200);

    // 若立即有数据（事件或消息），直接返回
    bool hasInstant = m_instantEvents.contains(user) && !m_instantEvents[user].isEmpty();
    QJsonArray probe;
    if (!hasInstant) {
        m_db->getMessagesSinceForUser(user, cursor, 1, probe);
    }
    if (hasInstant || !probe.isEmpty()) {
        QJsonObject data = buildPollDataForUser(user, cursor, limit);
        return QJsonObject{{"type","poll_events_response"},{"success",true},{"data", data}};
    }

    // 否则挂起请求：若已存在同用户挂起，则丢弃本次新请求（快速空响应），保留原挂起不变
    if (m_pendingPollRequests.contains(user)) {
        QJsonObject emptyData{{"messages", QJsonArray()}, {"instant_events", QJsonArray()}, {"next_cursor", cursor}, {"has_more", false}};
        return QJsonObject{{"type","poll_events_response"},{"success",true},{"data",emptyData}};
    }

    m_pendingPollRequests.insert(user, req);
    auto timer = new QTimer(this);
    timer->setSingleShot(true);
    QObject::connect(timer, &QTimer::timeout, this, [this, user]() {
        fulfillPendingPoll(user);
    });
    timer->start(timeoutSec * 1000);
    m_pendingPollTimers.insert(user, timer);

    // 返回空对象，表示异步应答
    return QJsonObject();
}

void ChatModule::enqueueInstantForPair(const QString &doctor, const QString &patient, const QJsonObject &event) {
    // 事件推送给双方
    m_instantEvents[doctor].enqueue(event);
    m_instantEvents[patient].enqueue(event);
    // 尝试唤醒挂起的轮询
    fulfillPendingPoll(doctor);
    fulfillPendingPoll(patient);
}

void ChatModule::fulfillPendingPoll(const QString &user) {
    if (!m_pendingPollRequests.contains(user)) return;
    QJsonObject orig = m_pendingPollRequests.take(user);
    if (m_pendingPollTimers.contains(user) && m_pendingPollTimers[user]) {
        m_pendingPollTimers[user]->stop();
        m_pendingPollTimers[user]->deleteLater();
        m_pendingPollTimers.remove(user);
    }
    const qint64 cursor = orig.value("cursor").toVariant().toLongLong();
    const int limit = qBound(1, orig.value("limit").toInt(50), 200);
    QJsonObject data = buildPollDataForUser(user, cursor, limit);
    QJsonObject resp{{"type","poll_events_response"},{"success",true},{"data", data}};
    reply(resp, orig);
}

QJsonObject ChatModule::buildPollDataForUser(const QString &user, qint64 cursor, int limit) {
    // 取瞬时事件（如果存在则全部取出）
    QJsonArray instant;
    if (m_instantEvents.contains(user)) {
        auto &q = m_instantEvents[user];
        while (!q.isEmpty()) instant.append(q.dequeue());
        if (q.isEmpty()) m_instantEvents.remove(user);
    }
    // 取新消息
    QJsonArray msgs;
    m_db->getMessagesSinceForUser(user, cursor, limit, msgs);
    qint64 nextCursor = cursor;
    if (!msgs.isEmpty()) nextCursor = msgs.last().toObject().value("id").toVariant().toLongLong();
    return QJsonObject{{"messages", msgs}, {"instant_events", instant}, {"next_cursor", nextCursor}, {"has_more", msgs.size()>=limit}};
}
