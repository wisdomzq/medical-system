#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QTcpSocket>

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

    // 获取所有医生排班信息
    QList<DoctorSchedule> getAllDoctorSchedules();

    // 根据医生编号挂号
    bool registerPatient(int doctorId, const QString& patientName, QString& errorMsg);

    // 将医生排班信息转为 JSON
    static QJsonObject doctorScheduleToJson(const DoctorSchedule& ds);

public slots:
    // 网络层信号对应的槽
    void onJsonReceived(const QJsonObject& json, QTcpSocket* socket);

signals:
    // 需要向客户端回复时发出的信号
    void sendJsonToClient(const QJsonObject& json, QTcpSocket* socket);
};