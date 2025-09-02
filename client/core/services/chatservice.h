#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QMap>
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
    // 轮询控制（由 Service 统一串联，避免 UI 层重复触发）
    void startPolling();
    void stopPolling();

    // 简易消息管理器（按会话缓存，升序 by id）
    QList<QJsonObject> messagesFor(const QString& doctorUser, const QString& patientUser) const;
    qint64 earliestIdFor(const QString& doctorUser, const QString& patientUser) const;

signals:
    void requestChatResult(bool ok, const QString& message);
    void acceptChatResult(bool ok, const QString& message);
    void sendMessageResult(bool ok, const QJsonObject& data);
    void historyReceived(const QJsonArray& messages, bool hasMore);
    void eventsReceived(const QJsonArray& messages, const QJsonArray& instantEvents, qint64 nextCursor, bool hasMore);
    void recentContactsReceived(const QJsonArray& contacts);

    // 新增：按会话整理后的消息推送
    void conversationHistoryLoaded(const QString& doctorUser, const QString& patientUser,
                                   const QList<QJsonObject>& pageAsc, bool hasMore, qint64 newEarliestId);
    void conversationUpserted(const QString& doctorUser, const QString& patientUser,
                              const QList<QJsonObject>& deltaAsc);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    void scheduleNextPoll(int delayMs = 0);
    void doPoll();
    static QString convKey(const QString& doctorUser, const QString& patientUser);
    void mergeAscending(const QString& key, const QList<QJsonObject>& pageAsc, QList<QJsonObject>* deltaOut = nullptr);

    CommunicationClient* m_client = nullptr;
    QString m_currentUser;
    // 轮询状态
    bool m_polling {false};
    bool m_pollInFlight {false};
    qint64 m_pollCursor {0};
    // 会话 -> 升序消息
    QMap<QString, QList<QJsonObject>> m_convMessages;
    // 会话 -> 最早 id（便于翻页）
    QMap<QString, qint64> m_earliestId;
};
