#pragma once
#include <QObject>
#include <QJsonObject>
class PatientInfoModule : public QObject {
    Q_OBJECT
public:
    explicit PatientInfoModule(QObject *parent=nullptr);
signals:
    void businessResponse(QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleGet(const QJsonObject &payload);
    void handleUpdate(const QJsonObject &payload);
    void reply(QJsonObject resp, const QJsonObject &orig);
};
