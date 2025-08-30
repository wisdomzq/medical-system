#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

// 将医生相关的三个子模块统一通过槽连接：profile/assignment/attendance
class DoctorRouterModule : public QObject {
    Q_OBJECT
public:
    explicit DoctorRouterModule(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void reply(QJsonObject resp, const QJsonObject &orig);
};
