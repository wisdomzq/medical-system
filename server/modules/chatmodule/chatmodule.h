#pragma once
#include <QObject>
#include <QJsonObject>
#include <QQueue>
#include <QHash>
#include <QPointer>
class QTimer;

class DBManager;

// 简化聊天模块：系统事件不入库，仅通过内存队列分发；消息入库
class ChatModule : public QObject {
    Q_OBJECT
public:
    explicit ChatModule(QObject *parent=nullptr);
    ~ChatModule();

signals:
    void businessResponse(QJsonObject payload);

private slots:
    void onRequest(const QJsonObject &payload);

private:
    void reply(QJsonObject resp, const QJsonObject &orig);

    // 系统事件队列：按参与用户聚合
    QHash<QString, QQueue<QJsonObject>> m_instantEvents; // key: username

    // 数据库
    DBManager *m_db = nullptr;

    // 处理动作
    QJsonObject handleRequestChat(const QJsonObject &req);
    QJsonObject handleAcceptChat(const QJsonObject &req);
    QJsonObject handleSendMessage(const QJsonObject &req);
    QJsonObject handleGetHistory(const QJsonObject &req);
    QJsonObject handlePollEvents(const QJsonObject &req);
    QJsonObject handleRecentContacts(const QJsonObject &req);

    void enqueueInstantForPair(const QString &doctor, const QString &patient, const QJsonObject &event);

    // 长轮询：挂起中的请求（按用户聚合）
    QHash<QString, QJsonObject> m_pendingPollRequests; // 保存原始请求（用于 request_uuid 回传与参数）
    QHash<QString, QPointer<QTimer>> m_pendingPollTimers; // 超时计时器

    // 工具方法
    void fulfillPendingPoll(const QString &user);
    QJsonObject buildPollDataForUser(const QString &user, qint64 cursor, int limit);
};
