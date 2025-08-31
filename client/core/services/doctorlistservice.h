#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

class DoctorListService : public QObject {
    Q_OBJECT
public:
    explicit DoctorListService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void fetchAllDoctors();

signals:
    void fetched(const QJsonArray& doctors);
    void failed(const QString& error);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
