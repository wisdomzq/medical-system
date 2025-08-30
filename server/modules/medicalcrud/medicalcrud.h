#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

// 汇总“create/update/lookup”类医疗数据动作，避免 main.cpp if-else
class MedicalCrudModule : public QObject {
    Q_OBJECT
public:
    explicit MedicalCrudModule(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleCreateRecord(const QJsonObject &payload);
    void handleUpdateRecord(const QJsonObject &payload);
    void handleGetAdvicesByRecord(const QJsonObject &payload);
    void handleCreateAdvice(const QJsonObject &payload);
    void handleCreatePrescription(const QJsonObject &payload);
    void handleGetPrescriptionsByPatient(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
