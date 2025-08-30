#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

// 将登录/注册改为通过信号槽接入 MessageRouter
class LoginRouter : public QObject {
    Q_OBJECT
public:
    explicit LoginRouter(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void reply(QJsonObject resp, const QJsonObject &orig);
};
