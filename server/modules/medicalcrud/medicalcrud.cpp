#include "medicalcrud.h"
#include "core/network/messagerouter.h"
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
    
    QJsonObject prescriptionData = payload.value("data").toObject();
    QJsonArray items = prescriptionData.value("items").toArray();
    
    qDebug() << "=== 处方创建调试 ===";
    qDebug() << "处方数据:" << prescriptionData;
    qDebug() << "处方项数量:" << items.size();
    
    // 验证处方数据
    if (items.isEmpty()) {
        qDebug() << "错误: 处方中没有药品项";
        QJsonObject out;
        out["type"] = "create_prescription_response";
        out["success"] = false;
        out["message"] = "处方中没有药品项";
        Log::result("MedicalCrud", false, "create_prescription - no items");
        reply(out, payload);
        return;
    }
    
    // 直接创建处方并获取ID
    int prescriptionId = db.createPrescriptionAndGetId(prescriptionData);
    bool ok = (prescriptionId > 0);
    qDebug() << "创建处方记录结果:" << ok << "处方ID:" << prescriptionId;
    
    QJsonObject out; 
    out["type"] = "create_prescription_response"; 
    out["success"] = ok;
    
    if (ok) {
        qDebug() << "新处方ID:" << prescriptionId;
        out["prescription_id"] = prescriptionId;
        
        // 添加处方项
        bool itemsOk = true;
        double totalAmount = 0.0;
        QString errorMsg;
        
        for (int i = 0; i < items.size(); ++i) {
            QJsonObject itemData = items[i].toObject();
            itemData["prescription_id"] = prescriptionId;
            
            qDebug() << "处理处方项" << (i+1) << ":" << itemData;
            
            // 计算总价
            double unitPrice = itemData.value("unit_price").toDouble();
            int quantity = itemData.value("quantity").toInt();
            double totalPrice = unitPrice * quantity;
            itemData["total_price"] = totalPrice;
            totalAmount += totalPrice;
            
            qDebug() << "单价:" << unitPrice << "数量:" << quantity << "小计:" << totalPrice;
            
            if (!db.addPrescriptionItem(itemData)) {
                itemsOk = false;
                errorMsg = QString("添加药品项失败: %1").arg(itemData.value("medication_name").toString());
                qDebug() << "添加处方项失败:" << errorMsg;
                break;
            } else {
                qDebug() << "成功添加处方项:" << itemData.value("medication_name").toString();
            }
        }
        
        qDebug() << "所有处方项添加结果:" << itemsOk << "总金额:" << totalAmount;
        
        if (!itemsOk) {
            out["success"] = false;
            out["message"] = errorMsg;
        } else {
            // 成功添加所有处方项，将状态更新为"已配药"
            bool statusUpdated = db.updatePrescriptionStatus(prescriptionId, "dispensed");
            qDebug() << "成功添加所有处方项，总金额:" << totalAmount << "状态更新结果:" << statusUpdated;
            out["message"] = "处方创建成功";
        }
        
        out["items_added"] = itemsOk;
        out["total_amount"] = totalAmount;
    } else {
        qDebug() << "创建处方记录失败";
        out["message"] = "创建处方记录失败";
    }
    
    qDebug() << "最终响应:" << out;
    Log::result("MedicalCrud", out["success"].toBool(), "create_prescription");
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
    emit businessResponse(resp);
}
