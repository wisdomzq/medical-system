#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
// 移除旧 QTcpSocket 依赖，改用 MessageRouter 广播机制

// 医生排班信息结构体
struct DoctorSchedule {
    int doctorId;
    QString department;
    QString jobNumber;
    QString name;
    QString profile;
    QString workTime;
    double fee;
    int maxPatientsPerDay;
    int reservedPatients;
    int remainingSlots() const { return maxPatientsPerDay - reservedPatients; }
};

class RegisterManager : public QObject {
    Q_OBJECT
public:
    explicit RegisterManager(QObject* parent = nullptr);

    QList<DoctorSchedule> getAllDoctorSchedules();

    bool registerPatient(int doctorId, const QString& patientName, QString& errorMsg);
    static QJsonObject doctorScheduleToJson(const DoctorSchedule& ds);
public slots:
    void onRequest(const QJsonObject& payload);
signals:
    void businessResponseReady(QJsonObject payload);
};