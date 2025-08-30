#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

class DoctorListModule : public QObject {
    Q_OBJECT
public:
    explicit DoctorListModule(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleGetAll(const QJsonObject &payload);
    void handleByDepartment(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
