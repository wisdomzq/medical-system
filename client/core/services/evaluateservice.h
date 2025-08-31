#pragma once

#include <QObject>

class CommunicationClient;

class EvaluateService : public QObject {
    Q_OBJECT
public:
    explicit EvaluateService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void fetchConfig(const QString& patientUsername);
    void recharge(const QString& patientUsername, double amount);

signals:
    void configReceived(double balance);
    void configFailed(const QString& error);
    void rechargeSucceeded(double newBalance, double amount);
    void rechargeFailed(const QString& error);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
