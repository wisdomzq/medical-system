#ifndef DOCTOR_ASSIGNMENT_MODULE_H
#define DOCTOR_ASSIGNMENT_MODULE_H

#include <QJsonObject>

// 医生端：排班/值班相关接口模块
// 说明：
// - 复用 doctors 表的 title 字段保存工作时间段（与 Profile 模块一致，格式 "HH:mm-HH:mm"）
// - 复用 doctors 表的 max_patients_per_day 作为单日接诊上限
// - 动作 action：
//   * get_doctor_assignment      -> 响应 type: get_doctor_assignment_response
//   * update_doctor_assignment   -> 响应 type: update_doctor_assignment_response
//   请求字段：username 或 doctor_username
//   更新字段：data{ work_time, max_patients_per_day }
class DoctorAssignmentModule {
public:
	QJsonObject handle(const QJsonObject& payload);

private:
	QJsonObject handleGet(const QJsonObject& payload);
	QJsonObject handleUpdate(const QJsonObject& payload);
};

#endif // DOCTOR_ASSIGNMENT_MODULE_H

