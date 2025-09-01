#include "advice.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/network/messagerouter.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDebug>
#include <algorithm>

AdviceModule::AdviceModule(QObject* parent) : QObject(parent) {
    // 连接消息路由器
    if (!connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &AdviceModule::onRequestReceived)) {
        Log::error("AdviceModule", "Failed to connect MessageRouter::requestReceived to AdviceModule::onRequestReceived");
    }
    if (!connect(this, &AdviceModule::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse)) {
        Log::error("AdviceModule", "Failed to connect AdviceModule::businessResponse to MessageRouter::onBusinessResponse");
    }
}

QJsonObject AdviceModule::handleAdviceRequest(const QJsonObject& request) {
    const QString action = request.value("action").toString();
    
    if (action == "advice_get_list") {
        QString patientUsername = request.value("patient_username").toString();
        return getAdvicesByPatient(patientUsername);
    }
    else if (action == "advice_get_details") {
        int adviceId = request.value("advice_id").toInt();
        return getAdviceDetails(adviceId);
    }
    
    // 未知的操作
    QJsonObject response;
    response["type"] = "advice_response";
    response["success"] = false;
    response["error"] = "Unknown action: " + action;
    return response;
}

QJsonObject AdviceModule::getAdvicesByPatient(const QString& patientUsername) {
    QJsonObject response;
    response["type"] = "advice_list_response";
    
    try {
        DBManager db(DatabaseConfig::getDatabasePath());
        
        // 先获取患者的医疗记录
        QJsonArray medicalRecords;
        if (!db.getMedicalRecordsByPatient(patientUsername, medicalRecords)) {
            response["success"] = false;
            response["error"] = "获取医疗记录失败";
            return response;
        }
        
        QJsonArray advices;
        int sequence = 1;
        
        // 遍历每个医疗记录，获取对应的医嘱
        for (const QJsonValue& recordValue : medicalRecords) {
            QJsonObject record = recordValue.toObject();
            int recordId = record.value("id").toInt();
            
            QJsonArray recordAdvices;
            if (db.getMedicalAdviceByRecord(recordId, recordAdvices)) {
                // 为每个医嘱添加医疗记录信息
                for (const QJsonValue& adviceValue : recordAdvices) {
                    QJsonObject advice = adviceValue.toObject();
                    advice["sequence"] = sequence++;
                    advice["visit_date"] = record.value("visit_date").toString();
                    advice["department"] = record.value("department").toString();
                    advice["doctor_name"] = record.value("doctor_name").toString();
                    advice["doctor_title"] = record.value("doctor_title").toString();
                    advice["diagnosis"] = record.value("diagnosis").toString();
                    
                    // 检查是否有关联的处方
                    QJsonArray prescriptions;
                    if (db.getPrescriptionsByPatient(patientUsername, prescriptions)) {
                        for (const QJsonValue& prescValue : prescriptions) {
                            QJsonObject prescription = prescValue.toObject();
                            if (prescription.value("record_id").toInt() == recordId) {
                                advice["prescription_id"] = prescription.value("id").toInt();
                                advice["has_prescription"] = true;
                                break;
                            }
                        }
                    }
                    
                    if (!advice.contains("has_prescription")) {
                        advice["has_prescription"] = false;
                        advice["prescription_id"] = 0;
                    }
                    
                    // 格式化医嘱类型显示
                    QString adviceType = advice.value("advice_type").toString();
                    if (adviceType == "medication") advice["advice_type_text"] = "用药建议";
                    else if (adviceType == "lifestyle") advice["advice_type_text"] = "生活方式";
                    else if (adviceType == "followup") advice["advice_type_text"] = "复诊建议";
                    else if (adviceType == "examination") advice["advice_type_text"] = "检查建议";
                    else advice["advice_type_text"] = adviceType;
                    
                    // 格式化优先级显示
                    QString priority = advice.value("priority").toString();
                    if (priority == "low") advice["priority_text"] = "低";
                    else if (priority == "normal") advice["priority_text"] = "普通";
                    else if (priority == "high") advice["priority_text"] = "重要";
                    else if (priority == "urgent") advice["priority_text"] = "紧急";
                    else advice["priority_text"] = priority;
                    
                    advices.append(advice);
                }
            }
        }
        
        // 按visit_date倒序排序（最新的在前）
        QVector<QJsonObject> advicesVector;
        for (const auto& adviceValue : advices) {
            advicesVector.append(adviceValue.toObject());
        }
        
        qInfo() << "[AdviceModule] 排序前医嘱数量:" << advicesVector.size();
        if (!advicesVector.isEmpty()) {
            qInfo() << "[AdviceModule] 排序前第一个医嘱日期:" << advicesVector.first().value("visit_date").toString();
            qInfo() << "[AdviceModule] 排序前最后一个医嘱日期:" << advicesVector.last().value("visit_date").toString();
        }
        
        std::sort(advicesVector.begin(), advicesVector.end(), [](const QJsonObject& a, const QJsonObject& b) {
            QString dateA = a.value("visit_date").toString();
            QString dateB = b.value("visit_date").toString();
            return dateA > dateB; // 倒序：最新的在前
        });
        
        if (!advicesVector.isEmpty()) {
            qInfo() << "[AdviceModule] 排序后第一个医嘱日期:" << advicesVector.first().value("visit_date").toString();
            qInfo() << "[AdviceModule] 排序后最后一个医嘱日期:" << advicesVector.last().value("visit_date").toString();
        }
        
        // 重新分配序号
        QJsonArray sortedAdvices;
        for (int i = 0; i < advicesVector.size(); ++i) {
            QJsonObject advice = advicesVector[i];
            advice["sequence"] = i + 1; // 重新按排序后的顺序分配序号
            sortedAdvices.append(advice);
        }
        
        response["success"] = true;
        response["data"] = sortedAdvices;
        
    } catch (const std::exception& e) {
        qDebug() << "获取患者医嘱异常:" << e.what();
        response["success"] = false;
        response["error"] = "服务器内部错误";
    }
    
    return response;
}

QJsonObject AdviceModule::getAdviceDetails(int adviceId) {
    QJsonObject response;
    response["type"] = "advice_details_response";
    
    try {
        DBManager db(DatabaseConfig::getDatabasePath());
        
        // 首先通过adviceId找到对应的医疗记录
        QJsonArray allRecords;
        // 这里我们需要遍历所有可能的患者记录来找到包含这个医嘱的记录
        // 由于DBManager没有直接的方法，我们简化处理
        
        response["success"] = false;
        response["error"] = "医嘱详情功能暂未完全实现";
        
    } catch (const std::exception& e) {
        qDebug() << "获取医嘱详情异常:" << e.what();
        response["success"] = false;
        response["error"] = "服务器内部错误";
    }
    
    return response;
}

void AdviceModule::onRequestReceived(const QJsonObject& payload) {
    const QString action = payload.value("action").toString();
    
    // 只处理医嘱相关的请求
    if (action.startsWith("advice_")) {
        QJsonObject response = handleAdviceRequest(payload);
        
        // 添加request_uuid字段
        if (payload.contains("uuid")) {
            response["request_uuid"] = payload.value("uuid").toString();
        }
    Log::response("Advice", response);
        // 发送响应
    emit businessResponse(response);
    }
}
