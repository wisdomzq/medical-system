#ifndef DOCTORINFO_H
#define DOCTORINFO_H

#include <QObject>
#include <QJsonObject>
#include "core/network/protocol.h"

class DoctorInfoModule : public QObject {
    Q_OBJECT

public:
    explicit DoctorInfoModule(QObject* parent = nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);

public:
    // 处理医生信息相关请求
    QJsonObject handleDoctorInfoRequest(const QJsonObject& request);

private:
    // 获取医生详细信息（包括排班信息）
    QJsonObject getDoctorDetails(const QString& doctorUsername);
    
    // 获取医生排班信息
    QJsonObject getDoctorSchedule(const QString& doctorUsername);

public slots:
    void onRequestReceived(const QJsonObject& payload);
};

#endif // DOCTORINFO_H
