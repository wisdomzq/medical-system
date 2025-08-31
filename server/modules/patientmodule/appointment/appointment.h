#pragma once
#include <QObject>
#include <QJsonObject>
class AppointmentModule : public QObject {
    Q_OBJECT
public:
    explicit AppointmentModule(QObject *parent=nullptr);
signals:
    void businessResponse(QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleCreate(const QJsonObject &payload);
    void handleListByPatient(const QJsonObject &payload);
    void handleListByDoctor(const QJsonObject &payload);
    void handleOverview(const QJsonObject &payload);
    void handleStats(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
