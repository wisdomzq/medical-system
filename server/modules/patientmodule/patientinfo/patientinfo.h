#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

class PatientInfoModule : public QObject {
    Q_OBJECT
public:
    explicit PatientInfoModule(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleGet(const QJsonObject &payload);
    void handleUpdate(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
