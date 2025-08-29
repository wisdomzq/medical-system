#include <QtGlobal>
#include <QCoreApplication>
#include <QDebug>
#include "core/network/src/protocol.h"
#include "core/network/src/server/communicationserver.h"
#include "core/network/src/server/messagerouter.h"
#include "modules/loginmodule/loginmodule.h"
#include "modules/patientmodule/medicine/medicine.h"
#include "core/database/database.h"
#include "core/database/database_config.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationServer server;
    LoginModule loginModule;
    MedicineModule medicineModule; // 负责药品相关请求

    QObject::connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
                     [&](const QJsonObject &payload) {
        QJsonObject responsePayload;
        const QString action = payload.value("action").toString();

        DBManager db(DatabaseConfig::getDatabasePath()); // 使用统一的数据库路径配置

        if (action == "login") {
            responsePayload = loginModule.handleLogin(payload);
        } else if (action == "register") {
            responsePayload = loginModule.handleRegister(payload);
        } else if (action == "get_doctor_info") {
            QString username = payload.value("username").toString();
            QJsonObject doctorInfo;
            bool ok = db.getDoctorInfo(username, doctorInfo);
            responsePayload["type"] = "doctor_info_response";
            responsePayload["success"] = ok;
            if (ok) {
                responsePayload["data"] = doctorInfo;
            }
        } else if (action == "update_doctor_info") {
            const QString username = payload.value("username").toString();
            QJsonObject data = payload.value("data").toObject();
            bool ok = db.updateDoctorInfo(username, data);
            responsePayload["type"] = "update_doctor_info_response";
            responsePayload["success"] = ok;
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
            bool ok = db.createAppointment(payload.value("data").toObject());
            responsePayload["type"] = "create_appointment_response";
            responsePayload["success"] = ok;
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
        } else if (action == "create_hospitalization") {
            bool ok = db.createHospitalization(payload.value("data").toObject());
            responsePayload["type"] = "create_hospitalization_response";
            responsePayload["success"] = ok;
        } else if (action == "get_hospitalizations_by_patient") {
            QJsonArray list; bool ok = db.getHospitalizationsByPatient(payload.value("patient_username").toString(), list);
            responsePayload["type"] = "hospitalizations_response"; responsePayload["success"] = ok; if(ok) responsePayload["data"] = list;
        } else if (action == "get_hospitalizations_by_doctor") {
            QJsonArray list; bool ok = db.getHospitalizationsByDoctor(payload.value("doctor_username").toString(), list);
            responsePayload["type"] = "hospitalizations_response"; responsePayload["success"] = ok; if(ok) responsePayload["data"] = list;
        } else if (action == "get_all_hospitalizations") {
            QJsonArray list; bool ok = db.getAllHospitalizations(list);
            responsePayload["type"] = "hospitalizations_response"; responsePayload["success"] = ok; if(ok) responsePayload["data"] = list;
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