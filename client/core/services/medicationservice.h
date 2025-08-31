#pragma once

#include <QObject>
#include <QJsonArray>

class CommunicationClient;

class MedicationService : public QObject {
    Q_OBJECT
public:
    explicit MedicationService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void fetchAll();
    void search(const QString& keyword);
    void searchRemote(const QString& keyword);

signals:
    void medicationsFetched(const QJsonArray& data);
    void fetchFailed(const QString& error);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
