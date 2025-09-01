#include "core/services/chatservice.h"
#include "core/network/communicationclient.h"
#include "core/logging/logging.h"
#include <QUuid>

ChatService::ChatService(CommunicationClient* client, const QString& currentUser, QObject* parent)
    : QObject(parent), m_client(client), m_currentUser(currentUser) {
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &ChatService::onJsonReceived);
}

void ChatService::requestChat(const QString& doctorUser, const QString& patientUser, const QString& note) {
    QJsonObject req{{"action","request_chat"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"user", m_currentUser},{"doctor_user",doctorUser},{"patient_user",patientUser},{"note",note}};
    Log::request("ChatService", req, "action", "request_chat");
    m_client->sendJson(req);
}

void ChatService::acceptChat(const QString& doctorUser, const QString& patientUser) {
    QJsonObject req{{"action","accept_chat"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"user", m_currentUser},{"doctor_user",doctorUser},{"patient_user",patientUser}};
    Log::request("ChatService", req, "action", "accept_chat");
    m_client->sendJson(req);
}

void ChatService::sendText(const QString& doctorUser, const QString& patientUser, const QString& text) {
    QJsonObject req{{"action","send_message"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"user", m_currentUser},{"doctor_user",doctorUser},{"patient_user",patientUser},
                    {"message_id",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"message_type","text"},{"text_content",text},{"file_metadata", QJsonValue()}};
    Log::request("ChatService", req, "action", "send_message");
    m_client->sendJson(req);
}

void ChatService::getHistory(const QString& doctorUser, const QString& patientUser, qint64 beforeId, int limit) {
    QJsonObject req{{"action","get_history_messages"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"doctor_user",doctorUser},{"patient_user",patientUser},{"before_id", (double)beforeId},{"limit", limit}};
    Log::request("ChatService", req, "action", "get_history_messages");
    m_client->sendJson(req);
}

void ChatService::pollEvents(qint64 cursor, int timeoutSec, int limit) {
    QJsonObject req{{"action","poll_events"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"user", m_currentUser},{"cursor", (double)cursor},{"timeout_sec", timeoutSec},{"limit", limit}};
    Log::request("ChatService", req, "action", "poll_events");
    m_client->sendJson(req);
}

void ChatService::recentContacts(int limit) {
    QJsonObject req{{"action","recent_contacts"},{"uuid",QUuid::createUuid().toString(QUuid::WithoutBraces)},
                    {"user", m_currentUser},{"limit", limit}};
    Log::request("ChatService", req, "action", "recent_contacts");
    m_client->sendJson(req);
}

void ChatService::onJsonReceived(const QJsonObject& obj) {
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
        emit historyReceived(data.value("messages").toArray(), data.value("has_more").toBool());
        return;
    }
    if (type == "poll_events_response") {
        QJsonObject data = obj.value("data").toObject();
        emit eventsReceived(data.value("messages").toArray(), data.value("instant_events").toArray(),
                            (qint64)data.value("next_cursor").toDouble(), data.value("has_more").toBool());
        return;
    }
    if (type == "recent_contacts_response") {
        QJsonObject data = obj.value("data").toObject();
        emit recentContactsReceived(data.value("contacts").toArray());
        return;
    }
}
