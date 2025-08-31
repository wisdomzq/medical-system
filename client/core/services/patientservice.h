#pragma once

#include <QObject>
#include <QJsonObject>

class CommunicationClient;

// 患者领域服务：封装患者信息获取/更新等逻辑
class PatientService : public QObject {
    Q_OBJECT
public:
    // 使用外部共享的 CommunicationClient，避免重复连接
    explicit PatientService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    // API
    void requestPatientInfo(const QString& username);
    void updatePatientInfo(const QString& username, const QJsonObject& data);

signals:
    void patientInfoReceived(const QJsonObject& data);
    void updatePatientInfoResult(bool success, const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
