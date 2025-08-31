#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

class AdviceService : public QObject {
    Q_OBJECT
public:
    explicit AdviceService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void fetchAdviceList(const QString& patientUsername);
    void fetchAdviceDetails(int adviceId);

signals:
    void listFetched(const QJsonArray& data);
    void listFailed(const QString& error);
    void detailsFetched(const QJsonObject& data);
    void detailsFailed(const QString& error);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
