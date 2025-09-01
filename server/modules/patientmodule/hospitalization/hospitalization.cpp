#include "hospitalization.h"
#include "core/network/messagerouter.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <algorithm>

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
    QJsonArray list; 
    bool ok = db.getHospitalizationsByPatient(payload.value("patient_username").toString(), list);
    
    if (ok) {
        // 转换为Vector进行排序
        QVector<QJsonObject> hospitalizationsVector;
        for (int i = 0; i < list.size(); ++i) {
            hospitalizationsVector.append(list[i].toObject());
        }
        
        // 按admission_date倒序排序（最新的在前）
        qInfo() << "[ HospitalizationModule ] 排序前住院记录数量:" << hospitalizationsVector.size();
        if (!hospitalizationsVector.isEmpty()) {
            qInfo() << "[ HospitalizationModule ] 排序前第一个住院日期:" << hospitalizationsVector.first().value("admission_date").toString();
            qInfo() << "[ HospitalizationModule ] 排序前最后一个住院日期:" << hospitalizationsVector.last().value("admission_date").toString();
        }
        
        std::sort(hospitalizationsVector.begin(), hospitalizationsVector.end(), [](const QJsonObject& a, const QJsonObject& b) {
            QString dateA = a.value("admission_date").toString();
            QString dateB = b.value("admission_date").toString();
            return dateA > dateB; // 倒序：最新的在前
        });
        
        if (!hospitalizationsVector.isEmpty()) {
            qInfo() << "[ HospitalizationModule ] 排序后第一个住院日期:" << hospitalizationsVector.first().value("admission_date").toString();
            qInfo() << "[ HospitalizationModule ] 排序后最后一个住院日期:" << hospitalizationsVector.last().value("admission_date").toString();
        }
        
        // 转换回QJsonArray
        QJsonArray sortedList;
        for (const auto& hospitalization : hospitalizationsVector) {
            sortedList.append(hospitalization);
        }
        list = sortedList;
    }
    
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
    emit businessResponse(resp);
}
