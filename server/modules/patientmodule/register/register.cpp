#include "register.h"
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include "core/database/database.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"

RegisterManager::RegisterManager(QObject* parent): QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &RegisterManager::onRequest);
    connect(this, &RegisterManager::businessResponseReady, this, [](QJsonObject payload){
        if (!payload.contains("request_uuid") && payload.contains("uuid"))
            payload.insert("request_uuid", payload.value("uuid").toString());
        MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, payload);
    });
}

// 获取所有医生排班信息
QList<DoctorSchedule> RegisterManager::getAllDoctorSchedules() {
    QList<DoctorSchedule> list;
    DBManager db("data/user.db");
    QJsonArray doctors;
    if (db.getAllDoctors(doctors)) {
        for (const auto &v : doctors) {
            QJsonObject o = v.toObject();
            DoctorSchedule ds{};
            ds.doctorId = o.value("id").toInt();
            ds.department = o.value("department").toString();
            ds.jobNumber = o.value("job_number").toString();
            ds.name = o.value("name").toString();
            ds.profile = o.value("profile").toString();
            ds.workTime = o.value("work_time").toString();
            ds.fee = o.value("fee").toDouble();
            ds.maxPatientsPerDay = o.value("max_per_day").toInt();
            ds.reservedPatients = o.value("reserved_patients").toInt();
            list.push_back(ds);
        }
    }
    return list;
}

// 挂号逻辑
bool RegisterManager::registerPatient(int doctorId, const QString& patientName, QString& errorMsg) {
    DBManager db("data/user.db");
    QJsonObject appt;
    appt["doctor_id"] = doctorId;
    appt["patient_name"] = patientName;
    appt["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    bool ok = db.createAppointment(appt);
    if (!ok) errorMsg = QStringLiteral("创建预约失败");
    return ok;
}

// 将医生排班信息转为 JSON
QJsonObject RegisterManager::doctorScheduleToJson(const DoctorSchedule& ds) {
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

void RegisterManager::onRequest(const QJsonObject& payload) {
    const QString action = payload.value("action").toString(payload.value("type").toString());
    if (action == "get_doctor_schedule") {
        auto list = getAllDoctorSchedules();
        QJsonArray arr; for (auto &ds : list) arr.append(doctorScheduleToJson(ds));
        QJsonObject resp; resp["type"] = "doctor_schedule_response"; resp["success"] = true; resp["data"] = arr;
        if (payload.contains("uuid")) resp["request_uuid"] = payload.value("uuid").toString();
        emit businessResponseReady(resp);
    } else if (action == "register_doctor") {
        int doctorId = payload.value("doctorId").toInt();
        QString patientName = payload.value("patientName").toString();
        QString err; bool ok = registerPatient(doctorId, patientName, err);
        QJsonObject resp; resp["type"] = "register_doctor_response"; resp["success"] = ok; if(!ok) resp["error"] = err; else resp["message"] = QStringLiteral("挂号成功");
        if (payload.contains("uuid")) resp["request_uuid"] = payload.value("uuid").toString();
        emit businessResponseReady(resp);
    }
}