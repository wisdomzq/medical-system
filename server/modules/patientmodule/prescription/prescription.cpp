#include "prescription.h"
#include "core/network/messagerouter.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

PrescriptionModule::PrescriptionModule(QObject *parent):QObject(parent) {
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &PrescriptionModule::onRequest)) {
        Log::error("PrescriptionModule", "Failed to connect MessageRouter::requestReceived to PrescriptionModule::onRequest");
    }
    if (!connect(this, &PrescriptionModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("PrescriptionModule", "Failed to connect PrescriptionModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

void PrescriptionModule::onRequest(const QJsonObject &payload) {
    const QString action = payload.value("action").toString();
    qInfo() << "[ PrescriptionModule ] 收到动作" << action;
    
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
    
    if (ok) {
        // 转换为Vector进行排序
        QVector<QJsonObject> prescriptionsVector;
        for (int i = 0; i < list.size(); ++i) {
            QJsonObject prescription = list[i].toObject();
            
            // 通过doctor_username获取科室信息
            QString doctorUsername = prescription.value("doctor_username").toString();
            QJsonObject doctorInfo;
            if (db.getDoctorInfo(doctorUsername, doctorInfo)) {
                prescription["department"] = doctorInfo.value("department").toString();
            } else {
                prescription["department"] = "未知科室";
            }
            
            prescriptionsVector.append(prescription);
        }
        
        // 按prescription_date倒序排序（最新的在前）
        qInfo() << "[ PrescriptionModule ] 排序前处方数量:" << prescriptionsVector.size();
        if (!prescriptionsVector.isEmpty()) {
            qInfo() << "[ PrescriptionModule ] 排序前第一个处方日期:" << prescriptionsVector.first().value("prescription_date").toString();
            qInfo() << "[ PrescriptionModule ] 排序前最后一个处方日期:" << prescriptionsVector.last().value("prescription_date").toString();
        }
        
        std::sort(prescriptionsVector.begin(), prescriptionsVector.end(), [](const QJsonObject& a, const QJsonObject& b) {
            QString dateA = a.value("prescription_date").toString();
            QString dateB = b.value("prescription_date").toString();
            return dateA > dateB; // 倒序：最新的在前
        });
        
        if (!prescriptionsVector.isEmpty()) {
            qInfo() << "[ PrescriptionModule ] 排序后第一个处方日期:" << prescriptionsVector.first().value("prescription_date").toString();
            qInfo() << "[ PrescriptionModule ] 排序后最后一个处方日期:" << prescriptionsVector.last().value("prescription_date").toString();
        }
        
        // 为每个处方添加序号
        QJsonArray sortedList;
        for (int i = 0; i < prescriptionsVector.size(); ++i) {
            QJsonObject prescription = prescriptionsVector[i];
            prescription["sequence"] = i + 1; // 序号从1开始
            sortedList.append(prescription);
        }
        
        list = sortedList;
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
    Log::response("Prescription", resp);
    emit businessResponse(resp);
}
