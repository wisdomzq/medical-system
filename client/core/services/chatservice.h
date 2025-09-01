#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
class CommunicationClient;

class ChatService : public QObject {
    Q_OBJECT
public:
    explicit ChatService(CommunicationClient* client, const QString& currentUser, QObject* parent=nullptr);

    void requestChat(const QString& doctorUser, const QString& patientUser, const QString& note = QString());
    void acceptChat(const QString& doctorUser, const QString& patientUser);
    void sendText(const QString& doctorUser, const QString& patientUser, const QString& text);
    void getHistory(const QString& doctorUser, const QString& patientUser, qint64 beforeId = 0, int limit = 20);
    void pollEvents(qint64 cursor, int timeoutSec = 1800, int limit = 50);
    void recentContacts(int limit = 20);

signals:
    void requestChatResult(bool ok, const QString& message);
    void acceptChatResult(bool ok, const QString& message);
    void sendMessageResult(bool ok, const QJsonObject& data);
    void historyReceived(const QJsonArray& messages, bool hasMore);
    void eventsReceived(const QJsonArray& messages, const QJsonArray& instantEvents, qint64 nextCursor, bool hasMore);
    void recentContactsReceived(const QJsonArray& contacts);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr;
    QString m_currentUser;
};
