#pragma once
#include <QObject>
#include <QJsonObject>

class HospitalizationModule : public QObject {
    Q_OBJECT
public:
    explicit HospitalizationModule(QObject *parent=nullptr);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleCreate(const QJsonObject &payload);
    void handleByPatient(const QJsonObject &payload);
    void handleByDoctor(const QJsonObject &payload);
    void handleAll(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
