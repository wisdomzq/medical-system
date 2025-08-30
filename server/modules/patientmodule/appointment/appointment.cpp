#include "appointment.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDate>
#include <QTime>
#include <QSqlQuery>

AppointmentModule::AppointmentModule(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &AppointmentModule::onRequest);
}

void AppointmentModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    if (a.startsWith("get_appointments_") || a == "create_appointment" || a.startsWith("get_doctor"))
        Log::request("Appointment", payload,
                     "doctor", payload.value("doctor_username").toString(),
                     "user", payload.value("username").toString());
    if (a == "create_appointment") return handleCreate(payload);
    if (a == "get_appointments_by_patient") return handleListByPatient(payload);
    if (a == "get_appointments_by_doctor") return handleListByDoctor(payload);
    if (a == "get_doctors_schedule_overview") return handleOverview(payload);
    if (a == "get_doctor_schedule_with_stats") return handleStats(payload);
}

void AppointmentModule::handleCreate(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject out; out["type"] = "create_appointment_response";
    QJsonObject data = payload.value("data").toObject();
    bool ok = db.createAppointment(data);
    out["success"] = ok;
    if (!ok) {
        // 诊断提示
        QString diag; QJsonObject tmp;
        if (!db.getDoctorInfo(data.value("doctor_username").toString(), tmp)) diag += QStringLiteral("医生不存在; ");
        if (!db.getPatientInfo(data.value("patient_username").toString(), tmp)) diag += QStringLiteral("患者不存在; ");
        if (diag.isEmpty()) diag = QStringLiteral("数据库插入失败");
        out["error"] = diag;
    }
    Log::result("Appointment", ok, "create_appointment");
    reply(out, payload);
}

void AppointmentModule::handleListByPatient(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray arr; bool ok = db.getAppointmentsByPatient(payload.value("username").toString(), arr);
    QJsonObject out; out["type"] = "appointments_response"; out["success"] = ok; if (ok) out["data"] = arr; else out["error"] = QStringLiteral("查询失败");
    Log::resultCount("Appointment", ok, arr.size(), "appointments_by_patient");
    reply(out, payload);
}

void AppointmentModule::handleListByDoctor(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray arr; bool ok = db.getAppointmentsByDoctor(payload.value("username").toString(), arr);
    QJsonObject out; out["type"] = "appointments_response"; out["success"] = ok; if (ok) out["data"] = arr; else out["error"] = QStringLiteral("查询失败");
    Log::resultCount("Appointment", ok, arr.size(), "appointments_by_doctor");
    reply(out, payload);
}

void AppointmentModule::handleOverview(const QJsonObject &payload) {
    Q_UNUSED(payload)
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray arr; bool ok = db.getAllDoctorsScheduleOverview(arr);
    QJsonObject out; out["type"] = "doctors_schedule_overview_response"; out["success"] = ok; if (ok) out["data"] = arr; else out["error"] = QStringLiteral("获取失败");
    Log::resultCount("Appointment", ok, arr.size(), "doctors_schedule_overview");
    reply(out, payload);
}

void AppointmentModule::handleStats(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray arr; bool ok = db.getDoctorScheduleWithAppointmentStats(payload.value("doctor_username").toString(), arr);
    QJsonObject out; out["type"] = "doctor_schedule_stats_response"; out["success"] = ok; if (ok) out["data"] = arr; else out["error"] = QStringLiteral("获取失败");
    Log::resultCount("Appointment", ok, arr.size(), "doctor_schedule_stats");
    reply(out, payload);
}

void AppointmentModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("Appointment", resp);
    MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
}
