#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

// 患者侧预约服务：封装医生列表、患者预约列表与创建预约
class PatientAppointmentService : public QObject {
    Q_OBJECT
public:
    explicit PatientAppointmentService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    // 获取所有医生（用于患者预约页展示）
    void fetchAllDoctors();

    // 获取指定患者的预约列表
    void fetchAppointmentsForPatient(const QString& patientUsername);

    // 创建预约
    void createAppointment(const QJsonObject& data, const QString& uuid = QString());

signals:
    void doctorsFetched(const QJsonArray& data);
    void doctorsFetchFailed(const QString& message);

    void appointmentsFetched(const QJsonArray& data);
    void appointmentsFetchFailed(const QString& message);

    void createSucceeded(const QString& message);
    void createFailed(const QString& message);
    
    // 预约数量变化通知
    void appointmentCountChanged(const QString& doctorUsername, int delta);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client = nullptr; // 非拥有
    QString m_lastProcessedUuid;
};
