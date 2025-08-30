#pragma once

#include <QJsonObject>

// 医生考勤模块：打卡/请假/销假/查询
class DoctorAttendanceModule {
public:
    QJsonObject handle(const QJsonObject &request);

private:
    QJsonObject handleCheckin(const QJsonObject &request);
    QJsonObject handleLeave(const QJsonObject &request);
    QJsonObject handleGetActiveLeaves(const QJsonObject &request);
    QJsonObject handleCancelLeave(const QJsonObject &request);
};
