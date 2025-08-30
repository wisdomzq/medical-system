#include <QtGlobal>
#include <QCoreApplication>
#include <QDebug>
#include "core/network/src/protocol.h"
#include "core/network/src/server/communicationserver.h"
#include "core/network/src/server/messagerouter.h"
#include "modules/loginmodule/loginmodule.h"
#include "modules/patientmodule/medicine/medicine.h"
#include "modules/patientmodule/evaluate/evaluate.h"
#include "modules/patientmodule/prescription/prescription.h"
#include "modules/patientmodule/advice/advice.h"
#include "modules/patientmodule/doctorinfo/doctorinfo.h"
#include "modules/patientmodule/register/register.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "modules/doctormodule/profile/profile.h"
#include "modules/doctormodule/assignment/assignment.h"
#include "modules/doctormodule/attendance/attendance.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationServer server;
    LoginModule loginModule;

    MedicineModule medicineModule; // 负责药品相关请求
    EvaluateModule evaluateModule; // 负责评价相关请求
    PrescriptionModule prescriptionModule; // 负责处方相关请求
    AdviceModule adviceModule; // 负责医嘱相关请求
    DoctorInfoModule doctorInfoModule; // 负责医生信息相关请求
    RegisterManager registerManager; // 负责挂号相关请求
    
    DoctorProfileModule doctorProfileModule;
    DoctorAssignmentModule doctorAssignmentModule;
    DoctorAttendanceModule doctorAttendanceModule;


    QObject::connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
                     [&](const QJsonObject &payload) {
        QJsonObject responsePayload;
        const QString action = payload.value("action").toString();
        qDebug() << "[MainServer] 收到请求，action:" << action;

        DBManager db(DatabaseConfig::getDatabasePath()); // 使用统一的数据库路径配置

        // 这些动作由特定模块处理，这里直接返回避免产生 unknown_response 干扰
        if(action == "get_medications" || action == "search_medications" || action == "search_medications_remote"||
           action.startsWith("evaluate_")||action.startsWith("prescription_")||action.startsWith("advice_")||action.startsWith("doctorinfo_")) {
            qDebug() << "[MainServer] 动作" << action << "由其他模块处理，跳过";
            return; // 不发送重复响应
        }

        if (action == "login") {
            responsePayload = loginModule.handleLogin(payload);
        } else if (action == "register") {
            responsePayload = loginModule.handleRegister(payload);
        } else if (action == "get_doctor_info" || action == "update_doctor_info") {
            responsePayload = doctorProfileModule.handle(payload);
        } else if (action == "get_doctor_assignment" || action == "update_doctor_assignment") {
            responsePayload = doctorAssignmentModule.handle(payload);
    } else if (action == "doctor_checkin" || action == "doctor_leave" || action == "get_active_leaves" || action == "cancel_leave" || action == "get_attendance_history") {
            responsePayload = doctorAttendanceModule.handle(payload);
        } else if (action == "get_patient_info") {
            QString username = payload.value("username").toString();
            QJsonObject patientInfo;
            bool ok = db.getPatientInfo(username, patientInfo);
            responsePayload["type"] = "patient_info_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = patientInfo;
            }
        } else if (action == "update_patient_info") {
            const QString username = payload.value("username").toString();
            QJsonObject data = payload.value("data").toObject();
            bool ok = db.updatePatientInfo(username, data);
            responsePayload["type"] = "update_patient_info_response";
            responsePayload["success"] = ok;
        } else if (action == "create_appointment") {
            QJsonObject data = payload.value("data").toObject();
            bool ok = db.createAppointment(data);
            responsePayload["type"] = "create_appointment_response";
            responsePayload["success"] = ok;
            if(!ok){
                // 简单再做一次可诊断检查：确认 patient 与 doctor 是否存在
                QString diag;
                DBManager diagDb(DatabaseConfig::getDatabasePath());
                QJsonObject doctorInfo, patientInfo;
                bool doctorExists = diagDb.getDoctorInfo(data.value("doctor_username").toString(), doctorInfo);
                bool patientExists = diagDb.getPatientInfo(data.value("patient_username").toString(), patientInfo);
                if(!doctorExists) diag += "医生不存在; ";
                if(!patientExists) diag += "患者不存在; ";
                if(diag.isEmpty()) diag = "数据库插入失败(可能字段不匹配或外键约束)";
                responsePayload["error"] = diag;
            }
        } else if (action == "get_appointments_by_patient") {
            QString username = payload.value("username").toString();
            QJsonArray appointments;
            bool ok = db.getAppointmentsByPatient(username, appointments);
            responsePayload["type"] = "appointments_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = appointments;
            }
        } else if (action == "get_appointments_by_doctor") {
            QString username = payload.value("username").toString();
            QJsonArray appointments;
            bool ok = db.getAppointmentsByDoctor(username, appointments);
            responsePayload["type"] = "appointments_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = appointments;
            }
        } else if (action == "get_all_doctors") {
            QJsonArray doctors;
            bool ok = db.getAllDoctors(doctors);
            responsePayload["type"] = "doctors_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = doctors;
            }
        } else if (action == "register_doctor") {
            qDebug() << "[MainServer] 处理挂号请求";
            // 使用RegisterManager的逻辑
            QString patientName = payload.value("patientName").toString();
            QString err; bool ok = false;
            
            // 获取医生列表（使用RegisterManager的getAllDoctorSchedules逻辑）
            QJsonArray doctors;
            if (db.getAllDoctors(doctors)) {
                QString docName = payload.value("doctor_name").toString();
                int doctorId = payload.value("doctorId").toInt();
                
                // 如果有明确的医生ID，优先使用
                if (doctorId > 0 && doctorId <= doctors.size()) {
                    QJsonObject doctor = doctors[doctorId - 1].toObject();
                    QString doctorUsername = doctor.value("username").toString();
                    QString department = doctor.value("department").toString();
                    double fee = doctor.value("consultation_fee").toDouble();
                    
                    // 创建预约
                    QJsonObject appt;
                    appt["patient_username"] = patientName;
                    appt["doctor_username"] = doctorUsername;
                    appt["appointment_date"] = QDate::currentDate().toString("yyyy-MM-dd");
                    appt["appointment_time"] = QTime::currentTime().toString("HH:mm");
                    appt["department"] = department;
                    appt["chief_complaint"] = QString("预约挂号 - %1").arg(doctor.value("name").toString());
                    appt["fee"] = fee;
                    
                    ok = db.createAppointment(appt);
                    if (!ok) {
                        err = "创建预约失败，请稍后重试";
                    }
                } else {
                    // 根据医生姓名查找
                    bool found = false;
                    for (int i = 0; i < doctors.size(); ++i) {
                        QJsonObject doctor = doctors[i].toObject();
                        if (doctor.value("name").toString() == docName || doctor.value("username").toString() == docName) {
                            QString doctorUsername = doctor.value("username").toString();
                            QString department = doctor.value("department").toString();
                            double fee = doctor.value("consultation_fee").toDouble();
                            
                            // 创建预约
                            QJsonObject appt;
                            appt["patient_username"] = patientName;
                            appt["doctor_username"] = doctorUsername;
                            appt["appointment_date"] = QDate::currentDate().toString("yyyy-MM-dd");
                            appt["appointment_time"] = QTime::currentTime().toString("HH:mm");
                            appt["department"] = department;
                            appt["chief_complaint"] = QString("预约挂号 - %1").arg(doctor.value("name").toString());
                            appt["fee"] = fee;
                            
                            ok = db.createAppointment(appt);
                            if (!ok) {
                                err = "创建预约失败，请稍后重试";
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        err = "医生不存在或序号无效";
                    }
                }
            } else {
                err = "获取医生列表失败";
            }
            
            responsePayload["type"] = "register_doctor_response";
            responsePayload["success"] = ok;
            if (!ok) {
                responsePayload["error"] = err;
            } else {
                responsePayload["message"] = "挂号成功";
                responsePayload["doctor_name"] = payload.value("doctor_name").toString();
                responsePayload["patient_name"] = patientName;
            }
        } else if (action == "get_doctors_schedule_overview") {
            QJsonArray doctorsSchedule;
            bool ok = db.getAllDoctorsScheduleOverview(doctorsSchedule);
            responsePayload["type"] = "doctors_schedule_overview_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = doctorsSchedule;
            }
        } else if (action == "get_doctor_schedule_with_stats") {
            QString doctorUsername = payload.value("doctor_username").toString();
            QJsonArray scheduleStats;
            bool ok = db.getDoctorScheduleWithAppointmentStats(doctorUsername, scheduleStats);
            responsePayload["type"] = "doctor_schedule_stats_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = scheduleStats;
            }
        } else if (action == "get_medical_records_by_patient") {
            QJsonArray records;
            bool ok = db.getMedicalRecordsByPatient(payload.value("patient_username").toString(), records);
            responsePayload["type"] = "medical_records_response";
            responsePayload["success"] = ok;
            if (ok) responsePayload["data"] = records;
        } else if (action == "get_medical_records_by_doctor") {
            QJsonArray records;
            bool ok = db.getMedicalRecordsByDoctor(payload.value("doctor_username").toString(), records);
            responsePayload["type"] = "medical_records_response";
            responsePayload["success"] = ok;
            if (ok) responsePayload["data"] = records;
        } else if (action == "create_medical_record") {
            bool ok = db.createMedicalRecord(payload.value("data").toObject());
            responsePayload["type"] = "create_medical_record_response";
            responsePayload["success"] = ok;
            if (ok) {
                // 获取 last_insert_rowid
                QSqlQuery q; q.exec("SELECT last_insert_rowid() AS id");
                if (q.next()) responsePayload["record_id"] = q.value("id").toInt();
            }
        } else if (action == "update_medical_record") {
            bool ok = db.updateMedicalRecord(payload.value("record_id").toInt(), payload.value("data").toObject());
            responsePayload["type"] = "update_medical_record_response";
            responsePayload["success"] = ok;
        } else if (action == "get_medical_advices_by_record") {
            QJsonArray advices; bool ok = db.getMedicalAdviceByRecord(payload.value("record_id").toInt(), advices);
            responsePayload["type"] = "medical_advices_response"; responsePayload["success"] = ok; if (ok) responsePayload["data"] = advices;
        } else if (action == "create_medical_advice") {
            bool ok = db.createMedicalAdvice(payload.value("data").toObject());
            responsePayload["type"] = "create_medical_advice_response";
            responsePayload["success"] = ok;
        } else if (action == "get_medications") {
            QJsonArray medications;
            bool ok = db.getMedications(medications);
            responsePayload["type"] = "medications_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = medications;
            }
        } else if (action == "create_hospitalization") {
            bool ok = db.createHospitalization(payload.value("data").toObject());
            responsePayload["type"] = "create_hospitalization_response";
            responsePayload["success"] = ok;
        } else if (action == "get_hospitalizations_by_patient") {
            QJsonArray list; 
            bool ok = db.getHospitalizationsByPatient(payload.value("patient_username").toString(), list);
            responsePayload["type"] = "hospitalizations_response"; 
            responsePayload["success"] = ok; 
            if(ok) responsePayload["data"] = list;
        } else if (action == "get_hospitalizations_by_doctor") {
            QJsonArray list; 
            bool ok = db.getHospitalizationsByDoctor(payload.value("doctor_username").toString(), list);
            responsePayload["type"] = "hospitalizations_response"; 
            responsePayload["success"] = ok; 
            if(ok) responsePayload["data"] = list;
        } else if (action == "get_all_hospitalizations") {
            QJsonArray list; 
            bool ok = db.getAllHospitalizations(list);
            responsePayload["type"] = "hospitalizations_response"; 
            responsePayload["success"] = ok; 
            if(ok) responsePayload["data"] = list;
        } else {
            responsePayload["type"] = "unknown_response";
            responsePayload["success"] = false;
            responsePayload["error"] = QStringLiteral("Unknown action: %1").arg(action);
        }

        // echo back uuid if present
        if (payload.contains("uuid"))
            responsePayload["request_uuid"] = payload.value("uuid").toString();

        MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, responsePayload);
    });

    if (server.listen(QHostAddress::Any, Protocol::SERVER_PORT)) {
        qDebug() << "Server started on port" << Protocol::SERVER_PORT;
    } else {
        qWarning() << "Server failed to start on port" << Protocol::SERVER_PORT;
    }
    return a.exec();
}