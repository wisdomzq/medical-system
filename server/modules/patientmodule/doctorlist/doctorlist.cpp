#include "doctorlist.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>

DoctorListModule::DoctorListModule(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &DoctorListModule::onRequest);
    connect(this, &DoctorListModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse);
}

void DoctorListModule::onRequest(const QJsonObject &payload) {
    const QString a = payload.value("action").toString();
    if (a == "get_all_doctors" || a == "get_doctors_by_department")
        Log::request("DoctorList", payload, "dept", payload.value("department").toString());
    if (a == "get_all_doctors") return handleGetAll(payload);
    if (a == "get_doctors_by_department") return handleByDepartment(payload);
}

void DoctorListModule::handleGetAll(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list; bool ok = db.getAllDoctors(list);
    QJsonObject out; out["type"] = "doctors_response"; out["success"] = ok; if (ok) out["data"] = list; else out["error"] = QStringLiteral("获取医生列表失败");
    Log::resultCount("DoctorList", ok, list.size(), "all");
    reply(out, payload);
}

void DoctorListModule::handleByDepartment(const QJsonObject &payload) {
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list; bool ok = db.getDoctorsByDepartment(payload.value("department").toString(), list);
    QJsonObject out; out["type"] = "doctors_response"; out["success"] = ok; if (ok) out["data"] = list; else out["error"] = QStringLiteral("获取医生列表失败");
    Log::resultCount("DoctorList", ok, list.size(), "by_dept");
    reply(out, payload);
}

void DoctorListModule::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("DoctorList", resp);
    emit businessResponse(Protocol::MessageType::JsonResponse, resp);
}
