#include "medicalcrud.h"
#include "core/network/messagerouter.h"
#include "core/network/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QSqlQuery>
#include <QJsonArray>
#include <QDebug>

MedicalCrudModule::MedicalCrudModule(QObject *parent):QObject(parent) {
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &MedicalCrudModule::onRequest)) {
        Log::error("MedicalCrudModule", "Failed to connect MessageRouter::requestReceived to MedicalCrudModule::onRequest");
    }
    if (!connect(this, &MedicalCrudModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("MedicalCrudModule", "Failed to connect MedicalCrudModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

void MedicalCrudModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    if (a.startsWith("create_medical_") || a.startsWith("update_medical_") || a.startsWith("get_medical_") || a.startsWith("create_prescription") || a.startsWith("get_prescriptions_"))
        Log::request("MedicalCrud", payload);
    if (a == "create_medical_record") return handleCreateRecord(payload);
    if (a == "update_medical_record") return handleUpdateRecord(payload);
    if (a == "get_medical_advices_by_record") return handleGetAdvicesByRecord(payload);
    if (a == "create_medical_advice") return handleCreateAdvice(payload);
    if (a == "create_prescription") return handleCreatePrescription(payload);
    if (a == "get_prescriptions_by_patient") return handleGetPrescriptionsByPatient(payload);
}

void MedicalCrudModule::handleCreateRecord(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    bool ok = db.createMedicalRecord(payload.value("data").toObject());
    QJsonObject out; out["type"] = "create_medical_record_response"; out["success"] = ok;
    if (ok) { QSqlQuery q; q.exec("SELECT last_insert_rowid() AS id"); if (q.next()) out["record_id"] = q.value("id").toInt(); }
    Log::result("MedicalCrud", ok, "create_medical_record");
    reply(out, payload);
}

void MedicalCrudModule::handleUpdateRecord(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    bool ok = db.updateMedicalRecord(payload.value("record_id").toInt(), payload.value("data").toObject());
    QJsonObject out; out["type"] = "update_medical_record_response"; out["success"] = ok;
    Log::result("MedicalCrud", ok, "update_medical_record");
    reply(out, payload);
}

void MedicalCrudModule::handleGetAdvicesByRecord(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath()); QJsonArray arr; bool ok = db.getMedicalAdviceByRecord(payload.value("record_id").toInt(), arr);
    QJsonObject out; out["type"] = "medical_advices_response"; out["success"] = ok; if (ok) out["data"] = arr;
    Log::resultCount("MedicalCrud", ok, arr.size(), "advices_by_record");
    reply(out, payload);
}

void MedicalCrudModule::handleCreateAdvice(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    bool ok = db.createMedicalAdvice(payload.value("data").toObject());
    QJsonObject out; out["type"] = "create_medical_advice_response"; out["success"] = ok; if (!ok) out["message"] = "Failed to create medical advice";
    Log::result("MedicalCrud", ok, "create_medical_advice");
    reply(out, payload);
}

void MedicalCrudModule::handleCreatePrescription(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    bool ok = db.createPrescription(payload.value("data").toObject());
    QJsonObject out; out["type"] = "create_prescription_response"; out["success"] = ok; if (!ok) out["message"] = "Failed to create prescription";
    Log::result("MedicalCrud", ok, "create_prescription");
    reply(out, payload);
}

void MedicalCrudModule::handleGetPrescriptionsByPatient(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray arr; bool ok = db.getPrescriptionsByPatient(payload.value("patient_username").toString(), arr);
    QJsonObject out; out["type"] = "prescriptions_response"; out["success"] = ok; if (ok) out["data"] = arr;
    Log::resultCount("MedicalCrud", ok, arr.size(), "prescriptions_by_patient");
    reply(out, payload);
}

void MedicalCrudModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("MedicalCrud", resp);
    emit businessResponse(Protocol::MessageType::JsonResponse, resp);
}
