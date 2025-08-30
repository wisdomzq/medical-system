#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "core/network/src/protocol.h"

class MedicalRecordModule : public QObject
{
    Q_OBJECT

public:
    explicit MedicalRecordModule(QObject *parent = nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);

private slots:
    void onRequest(const QJsonObject &payload);

private:
    void handleGetMedicalRecords(const QJsonObject &payload);
    void handleGetMedicalRecordDetails(const QJsonObject &payload);
    void sendResponse(QJsonObject resp, const QJsonObject &orig);
};
