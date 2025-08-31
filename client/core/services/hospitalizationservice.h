#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

class HospitalizationService : public QObject {
    Q_OBJECT
public:
    explicit HospitalizationService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void fetchByPatient(const QString& patientUsername);
    void fetchByDoctor(const QString& doctorUsername);

signals:
    void fetched(const QJsonArray& data);
    void fetchFailed(const QString& error);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
