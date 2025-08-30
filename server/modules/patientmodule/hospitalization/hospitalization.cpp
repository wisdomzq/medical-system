#include "hospitalization.h"
#include "core/network/messagerouter.h"
#include "core/network/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>

HospitalizationModule::HospitalizationModule(QObject *parent):QObject(parent) {
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &HospitalizationModule::onRequest)) {
        Log::error("HospitalizationModule", "Failed to connect MessageRouter::requestReceived to HospitalizationModule::onRequest");
    }
    if (!connect(this, &HospitalizationModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("HospitalizationModule", "Failed to connect HospitalizationModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

void HospitalizationModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    if (a.startsWith("get_hospitalizations_") || a == "create_hospitalization")
        Log::request("Hospitalization", payload,
                     "patient", payload.value("patient_username").toString(),
                     "doctor", payload.value("doctor_username").toString());
    if (a == "create_hospitalization") return handleCreate(payload);
    if (a == "get_hospitalizations_by_patient") return handleByPatient(payload);
    if (a == "get_hospitalizations_by_doctor") return handleByDoctor(payload);
    if (a == "get_all_hospitalizations") return handleAll(payload);
}

void HospitalizationModule::handleCreate(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    bool ok = db.createHospitalization(payload.value("data").toObject());
    QJsonObject out; out["type"] = "create_hospitalization_response"; out["success"] = ok; if (!ok) out["error"] = QStringLiteral("创建失败");
    Log::result("Hospitalization", ok, "create_hospitalization");
    reply(out, payload);
}

void HospitalizationModule::handleByPatient(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list; bool ok = db.getHospitalizationsByPatient(payload.value("patient_username").toString(), list);
    QJsonObject out; out["type"] = "hospitalizations_response"; out["success"] = ok; if (ok) out["data"] = list; else out["error"] = QStringLiteral("查询失败");
    Log::resultCount("Hospitalization", ok, list.size(), "by_patient");
    reply(out, payload);
}

void HospitalizationModule::handleByDoctor(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list; bool ok = db.getHospitalizationsByDoctor(payload.value("doctor_username").toString(), list);
    QJsonObject out; out["type"] = "hospitalizations_response"; out["success"] = ok; if (ok) out["data"] = list; else out["error"] = QStringLiteral("查询失败");
    Log::resultCount("Hospitalization", ok, list.size(), "by_doctor");
    reply(out, payload);
}

void HospitalizationModule::handleAll(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list; bool ok = db.getAllHospitalizations(list);
    QJsonObject out; out["type"] = "hospitalizations_response"; out["success"] = ok; if (ok) out["data"] = list; else out["error"] = QStringLiteral("查询失败");
    Log::resultCount("Hospitalization", ok, list.size(), "all");
    reply(out, payload);
}

void HospitalizationModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("Hospitalization", resp);
    emit businessResponse(Protocol::MessageType::JsonResponse, resp);
}
