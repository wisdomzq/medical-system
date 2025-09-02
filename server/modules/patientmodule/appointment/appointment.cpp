#include "appointment.h"
#include "core/network/messagerouter.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDate>
#include <QTime>
#include <QSqlQuery>

AppointmentModule::AppointmentModule(QObject *parent):QObject(parent) {
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &AppointmentModule::onRequest)) {
        Log::error("AppointmentModule", "Failed to connect MessageRouter::requestReceived to AppointmentModule::onRequest");
    }
    if (!connect(this, &AppointmentModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("AppointmentModule", "Failed to connect AppointmentModule::businessResponse to MessageRouter::onBusinessResponse");
    }
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
    if (a == "update_appointment_status") return handleUpdateStatus(payload);
}

void AppointmentModule::handleCreate(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject out; out["type"] = "create_appointment_response";
    QJsonObject data = payload.value("data").toObject();
    bool ok = db.createAppointment(data);
    out["success"] = ok;
    if (!ok) {
        // 详细诊断提示
        QString diag; QJsonObject tmp;
        if (!db.getDoctorInfo(data.value("doctor_username").toString(), tmp)) {
            diag += QStringLiteral("医生不存在; ");
        } else {
            // 检查预约是否已满 - 使用数据库管理器的内部机制
            // 由于createAppointment已经做了验证，这里主要提供用户友好的错误信息
            diag = QStringLiteral("当前医生预约数量已达上限，请等待或选择其他医生挂号");
        }
        if (!db.getPatientInfo(data.value("patient_username").toString(), tmp)) {
            if (diag.isEmpty()) diag += QStringLiteral("患者不存在; ");
        }
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

void AppointmentModule::handleUpdateStatus(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    const QJsonObject data = payload.value("data").toObject();
    int apptId = data.value("appointment_id").toInt();
    QString status = data.value("status").toString();
    bool ok = apptId > 0 && !status.isEmpty() && db.updateAppointmentStatus(apptId, status);
    QJsonObject out; out["type"] = "update_appointment_status_response"; out["success"] = ok; if (!ok) out["error"] = QStringLiteral("更新失败");
    QJsonObject ret; ret["appointment_id"] = apptId; ret["status"] = status; out["data"] = ret;
    Log::result("Appointment", ok, "update_appointment_status");
    reply(out, payload);
    
    // 如果状态更新为completed，广播一个预约完成通知
    if (ok && status == "completed") {
        // 广播预约完成通知，让其他客户端知道预约数量发生变化
        QJsonObject notification;
        notification["type"] = "appointment_completed_notification";
        notification["appointment_id"] = apptId;
        notification["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        
        // 通过查询获取医生用户名和预约日期
        QJsonArray patientAppointments;
        QString allDoctors = ""; // 空字符串表示获取所有医生
        if (db.getAppointmentsByDoctor(allDoctors, patientAppointments)) {
            for (const auto& apptData : patientAppointments) {
                QJsonObject appt = apptData.toObject();
                if (appt.value("id").toInt() == apptId) {
                    notification["doctor_username"] = appt.value("doctor_username").toString();
                    notification["appointment_date"] = appt.value("appointment_date").toString();
                    break;
                }
            }
        }
        
        // 发送给消息路由器进行广播
        emit businessResponse(notification);
    }
}

void AppointmentModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("Appointment", resp);
    emit businessResponse(resp);
}
