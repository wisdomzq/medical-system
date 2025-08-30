#include "patientinfo.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QSqlQuery>

PatientInfoModule::PatientInfoModule(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &PatientInfoModule::onRequest);
}

void PatientInfoModule::onRequest(const QJsonObject &payload) {
    const QString action = payload.value("action").toString();
    if (action == "get_patient_info" || action == "update_patient_info")
        Log::request("PatientInfo", payload, "username", payload.value("username").toString());
    if (action == "get_patient_info") return handleGet(payload);
    if (action == "update_patient_info") return handleUpdate(payload);
}

void PatientInfoModule::handleGet(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject out; out["type"] = "patient_info_response";
    QJsonObject data; bool ok = db.getPatientInfo(payload.value("username").toString(), data);
    out["success"] = ok; if (ok) out["data"] = data; else out["error"] = QStringLiteral("获取患者信息失败");
    Log::result("PatientInfo", ok, "get_patient_info");
    reply(out, payload);
}

void PatientInfoModule::handleUpdate(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject out; out["type"] = "update_patient_info_response";
    bool ok = db.updatePatientInfo(payload.value("username").toString(), payload.value("data").toObject());
    out["success"] = ok; if (!ok) out["error"] = QStringLiteral("更新失败");
    Log::result("PatientInfo", ok, "update_patient_info");
    reply(out, payload);
}

void PatientInfoModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("PatientInfo", resp);
    MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
}
