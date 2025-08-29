#include <QtGlobal>
#include <QCoreApplication>
#include <QDebug>
#include "core/network/src/protocol.h"
#include "core/network/src/server/communicationserver.h"
#include "core/network/src/server/messagerouter.h"
#include "modules/loginmodule/loginmodule.h"
#include "core/database/database.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationServer server;
    LoginModule loginModule;

    QObject::connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
                     [&](const QJsonObject &payload) {
        QJsonObject responsePayload;
        const QString action = payload.value("action").toString();

        if (action == "login") {
            responsePayload = loginModule.handleLogin(payload);
        } else if (action == "register") {
            responsePayload = loginModule.handleRegister(payload);
        } else if (action == "get_doctor_info") {
            DBManager db("data/user.db");
            QString username = payload.value("username").toString();
            QJsonObject dataObj;
            QString department, phone;
            bool ok = db.getDoctorDetails(username, department, phone);
            responsePayload["type"] = "doctor_info_response";
            responsePayload["success"] = ok;
            if (ok) {
                dataObj["name"] = username;
                dataObj["department"] = department;
                dataObj["phone"] = phone;
                responsePayload["data"] = dataObj;
            }
        } else if (action == "update_doctor_info") {
            DBManager db("data/user.db");
            const QString username = payload.value("username").toString();
            QJsonObject data = payload.value("data").toObject();
            const QString newName = data.value("name").toString();
            const QString department = data.value("department").toString();
            const QString phone = data.value("phone").toString();
            bool ok = db.updateDoctorProfile(username, newName, department, phone);
            responsePayload["type"] = "update_doctor_info_response";
            responsePayload["success"] = ok;
        } else if (action == "get_patient_info") {
            DBManager db("data/user.db");
            QString username = payload.value("username").toString();
            QJsonObject dataObj;
            int age; QString phone, address;
            bool ok = db.getPatientDetails(username, age, phone, address);
            responsePayload["type"] = "patient_info_response";
            responsePayload["success"] = ok;
            if (ok) {
                dataObj["name"] = username;
                dataObj["age"] = age;
                dataObj["phone"] = phone;
                dataObj["address"] = address;
                responsePayload["data"] = dataObj;
            }
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
