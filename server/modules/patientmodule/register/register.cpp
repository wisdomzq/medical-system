#include "register.h"
#include "registerdb.h" // 假设你的数据库接口在这个头文件
#include <QJsonArray>
#include <QTcpSocket>

// 构造函数
RegisterManager::RegisterManager(QObject* parent)
    : QObject(parent)
{
}

// 获取所有医生排班信息
QList<DoctorSchedule> RegisterManager::getAllDoctorSchedules()
{
    // 直接调用数据库层的函数
    return RegisterDB::getAllDoctorSchedules();
}

// 挂号逻辑
bool RegisterManager::registerPatient(int doctorId, const QString& patientName, QString& errorMsg)
{
    // 直接调用数据库层的函数
    return RegisterDB::registerPatient(doctorId, patientName, errorMsg);
}

// 将医生排班信息转为 JSON
QJsonObject RegisterManager::doctorScheduleToJson(const DoctorSchedule& ds)
{
    QJsonObject obj;
    obj["doctorId"] = ds.doctorId;
    obj["department"] = ds.department;
    obj["jobNumber"] = ds.jobNumber;
    obj["name"] = ds.name;
    obj["profile"] = ds.profile;
    obj["workTime"] = ds.workTime;
    obj["fee"] = ds.fee;
    obj["maxPatientsPerDay"] = ds.maxPatientsPerDay;
    obj["reservedPatients"] = ds.reservedPatients;
    obj["remainingSlots"] = ds.remainingSlots();
    return obj;
}

// 槽函数：处理网络层广播的 JSON 请求
void RegisterManager::onJsonReceived(const QJsonObject& json, QTcpSocket* socket)
{
    if (!json.contains("type"))
        return;

    QString type = json.value("type").toString();

    if (type == "get_doctor_schedule") {
        // 查询所有医生排班
        QList<DoctorSchedule> schedules = getAllDoctorSchedules();
        QJsonArray arr;
        for (const auto& ds : schedules) {
            arr.append(doctorScheduleToJson(ds));
        }
        QJsonObject resp;
        resp["type"] = "doctor_schedule_response";
        resp["success"] = true;
        resp["data"] = arr;
        emit sendJsonToClient(resp, socket);
    } else if (type == "register_doctor") {
        // 挂号
        int doctorId = json.value("doctorId").toInt();
        QString patientName = json.value("patientName").toString();
        QString errorMsg;
        bool ok = registerPatient(doctorId, patientName, errorMsg);
        QJsonObject resp;
        resp["type"] = "register_doctor_response";
        resp["success"] = ok;
        resp["message"] = ok ? "挂号成功" : errorMsg;
        emit sendJsonToClient(resp, socket);
    }
}