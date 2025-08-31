#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

// 预约服务：封装预约查询等逻辑
class AppointmentService : public QObject {
    Q_OBJECT
public:
    explicit AppointmentService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    // 查询指定医生的预约列表
    void fetchByDoctor(const QString& doctorUsername);

signals:
    void fetched(const QJsonArray& data);
    void fetchFailed(const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
};
