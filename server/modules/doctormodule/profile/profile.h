#pragma once

#include <QJsonObject>

// 医生个人信息模块：处理 get_doctor_info / update_doctor_info
class DoctorProfileModule {
public:
    // 统一入口：根据 action 分发
    QJsonObject handle(const QJsonObject& request);

    // 具体处理函数
    QJsonObject handleGet(const QJsonObject& request);
    QJsonObject handleUpdate(const QJsonObject& request);
};
