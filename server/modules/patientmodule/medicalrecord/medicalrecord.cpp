#include "medicalrecord.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDebug>

MedicalRecordModule::MedicalRecordModule(QObject *parent) : QObject(parent)
{
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &MedicalRecordModule::onRequest)) {
        Log::error("MedicalRecordModule", "Failed to connect MessageRouter::requestReceived to MedicalRecordModule::onRequest");
    }
    if (!connect(this, &MedicalRecordModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("MedicalRecordModule", "Failed to connect MedicalRecordModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

void MedicalRecordModule::onRequest(const QJsonObject &payload)
{
    const QString action = payload.value("action").toString();
    qInfo() << "[MedicalRecordModule] 收到动作" << action;
    
    if (action == "get_medical_records" || action == "get_medical_records_by_patient") {
        return handleGetMedicalRecords(payload);
    }
    if (action == "get_medical_record_details") {
        return handleGetMedicalRecordDetails(payload);
    }
    if (action == "get_medical_records_by_doctor") {
        // 兼容旧接口名称，直接透传到 DBManager 的医生查询
        QJsonObject resp; resp["type"] = "medical_records_response";
        DBManager db(DatabaseConfig::getDatabasePath());
        QJsonArray records; bool ok = db.getMedicalRecordsByDoctor(payload.value("doctor_username").toString(), records);
        resp["success"] = ok; if (ok) resp["data"] = records; else resp["error"] = "获取医生病例记录失败";
        return sendResponse(resp, payload);
    }
}

void MedicalRecordModule::handleGetMedicalRecords(const QJsonObject &payload)
{
    QString patientUsername = payload.value("patient_username").toString();
    
    QJsonObject resp;
    resp["type"] = "medical_records_response";
    
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray records;
    bool ok = db.getMedicalRecordsByPatient(patientUsername, records);
    
    if (ok) {
        // 对返回的数据进行简化，只保留表格显示需要的字段
        QJsonArray simplifiedRecords;
        for (int i = 0; i < records.size(); ++i) {
            QJsonObject record = records[i].toObject();
            QJsonObject simplified;
            simplified["id"] = record.value("id").toInt();
            simplified["visit_date"] = record.value("visit_date").toString();
            simplified["department"] = record.value("department").toString();
            simplified["doctor_name"] = record.value("doctor_name").toString();
            simplified["diagnosis"] = record.value("diagnosis").toString();
            simplified["doctor_title"] = record.value("doctor_title").toString();
            simplifiedRecords.append(simplified);
        }
        
        resp["success"] = true;
        resp["data"] = simplifiedRecords;
    } else {
        resp["success"] = false;
        resp["error"] = "获取病例记录失败";
    }
    
    sendResponse(resp, payload);
}

void MedicalRecordModule::handleGetMedicalRecordDetails(const QJsonObject &payload)
{
    int recordId = payload.value("record_id").toInt();
    
    QJsonObject resp;
    resp["type"] = "medical_record_details_response";
    
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray records;
    
    // 通过患者用户名获取所有记录，然后筛选指定的记录
    QString patientUsername = payload.value("patient_username").toString();
    bool ok = db.getMedicalRecordsByPatient(patientUsername, records);
    
    if (ok) {
        for (int i = 0; i < records.size(); ++i) {
            QJsonObject record = records[i].toObject();
            if (record.value("id").toInt() == recordId) {
                resp["success"] = true;
                resp["data"] = record;
                sendResponse(resp, payload);
                return;
            }
        }
        resp["success"] = false;
        resp["error"] = "未找到指定的病例记录";
    } else {
        resp["success"] = false;
        resp["error"] = "获取病例详情失败";
    }
    
    sendResponse(resp, payload);
}

void MedicalRecordModule::sendResponse(QJsonObject resp, const QJsonObject &orig)
{
    if (orig.contains("uuid")) {
        resp["request_uuid"] = orig.value("uuid").toString();
    }
    Log::response("MedicalRecord", resp);
    emit businessResponse(Protocol::MessageType::JsonResponse, resp);
}
