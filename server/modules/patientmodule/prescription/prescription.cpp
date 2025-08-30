#include "prescription.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include <QJsonArray>
#include <QDebug>

PrescriptionModule::PrescriptionModule(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &PrescriptionModule::onRequest);
}

void PrescriptionModule::onRequest(const QJsonObject &payload) {
    const QString action = payload.value("action").toString();
    qInfo() << "[PrescriptionModule] 收到动作" << action;
    
    if (action == "prescription_get_list") return handleGetList(payload);
    if (action == "prescription_get_details") return handleGetDetails(payload);
}

void PrescriptionModule::handleGetList(const QJsonObject &payload) {
    const QString patient = payload.value("patient_username").toString();
    if (patient.isEmpty()) {
        QJsonObject resp;
        resp["type"] = "prescription_list_response";
        resp["success"] = false;
        resp["error"] = "缺少患者用户名参数";
        return sendResponse(resp, payload);
    }
    
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonArray list;
    bool ok = db.getPrescriptionsByPatient(patient, list);
    
    // 为每个处方添加序号和科室信息
    for (int i = 0; i < list.size(); ++i) {
        QJsonObject prescription = list[i].toObject();
        prescription["sequence"] = i + 1; // 序号从1开始
        
        // 通过doctor_username获取科室信息
        QString doctorUsername = prescription.value("doctor_username").toString();
        QJsonObject doctorInfo;
        if (db.getDoctorInfo(doctorUsername, doctorInfo)) {
            prescription["department"] = doctorInfo.value("department").toString();
        } else {
            prescription["department"] = "未知科室";
        }
        
        list[i] = prescription;
    }
    
    QJsonObject resp;
    resp["type"] = "prescription_list_response";
    resp["success"] = ok;
    if (ok) {
        resp["data"] = list;
        resp["count"] = list.size();
    } else {
        resp["error"] = "获取处方列表失败";
    }
    
    sendResponse(resp, payload);
}

void PrescriptionModule::handleGetDetails(const QJsonObject &payload) {
    int prescriptionId = payload.value("prescription_id").toInt();
    if (prescriptionId <= 0) {
        QJsonObject resp;
        resp["type"] = "prescription_details_response";
        resp["success"] = false;
        resp["error"] = "无效的处方ID";
        return sendResponse(resp, payload);
    }
    
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject details;
    bool ok = db.getPrescriptionDetails(prescriptionId, details);
    
    if (ok) {
        // 添加科室信息
        QString doctorUsername = details.value("doctor_username").toString();
        QJsonObject doctorInfo;
        if (db.getDoctorInfo(doctorUsername, doctorInfo)) {
            details["department"] = doctorInfo.value("department").toString();
        } else {
            details["department"] = "未知科室";
        }
        
        // 格式化状态显示
        QString status = details.value("status").toString();
        if (status == "pending") details["status_text"] = "待配药";
        else if (status == "dispensed") details["status_text"] = "已配药";
        else if (status == "cancelled") details["status_text"] = "已取消";
        else details["status_text"] = "未知状态";
    }
    
    QJsonObject resp;
    resp["type"] = "prescription_details_response";
    resp["success"] = ok;
    if (ok) {
        resp["data"] = details;
    } else {
        resp["error"] = "获取处方详情失败";
    }
    
    sendResponse(resp, payload);
}

void PrescriptionModule::sendResponse(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
}
