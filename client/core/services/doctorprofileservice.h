#pragma once

#include <QObject>
#include <QJsonObject>

class CommunicationClient;

// 医生个人信息服务：封装获取与更新
class DoctorProfileService : public QObject {
    Q_OBJECT
public:
    explicit DoctorProfileService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    void requestDoctorInfo(const QString& username);
    void updateDoctorInfo(const QString& username, const QJsonObject& data);

signals:
    void infoReceived(const QJsonObject& data);
    void infoFailed(const QString& message);
    void updateResult(bool success, const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
