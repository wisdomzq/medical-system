#include "doctorinfo.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/logging/logging.h"
#include <QJsonArray>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>

#include "doctorinfo.h"
#include "core/database/database.h"
#include "core/database/database_config.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include <QJsonArray>
#include <QDebug>

DoctorInfoModule::DoctorInfoModule(QObject* parent) : QObject(parent) {
    // 连接消息路由器
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &DoctorInfoModule::onRequestReceived);
}

QJsonObject DoctorInfoModule::handleDoctorInfoRequest(const QJsonObject& request) {
    const QString action = request.value("action").toString();
    
    if (action == "doctorinfo_get_details") {
        QString doctorUsername = request.value("doctor_username").toString();
        return getDoctorDetails(doctorUsername);
    }
    else if (action == "doctorinfo_get_schedule") {
        QString doctorUsername = request.value("doctor_username").toString();
        return getDoctorSchedule(doctorUsername);
    }
    
    // 未知的操作
    QJsonObject response;
    response["type"] = "doctorinfo_response";
    response["success"] = false;
    response["error"] = "Unknown action: " + action;
    return response;
}

QJsonObject DoctorInfoModule::getDoctorDetails(const QString& doctorUsername) {
    QJsonObject response;
    response["type"] = "doctorinfo_details_response";
    
    try {
        DBManager db(DatabaseConfig::getDatabasePath());
        
        // 获取医生基本信息
        QJsonObject doctorInfo;
        if (!db.getDoctorInfo(doctorUsername, doctorInfo)) {
            response["success"] = false;
            response["error"] = "获取医生信息失败";
            return response;
        }
        
        // 获取医生统计信息
        QJsonObject statistics;
        if (db.getDoctorStatistics(doctorUsername, statistics)) {
            doctorInfo["statistics"] = statistics;
        }
        
        // 获取医生排班信息
        QJsonArray schedules;
        if (db.getDoctorSchedules(doctorUsername, schedules)) {
            doctorInfo["schedules"] = schedules;
        }
        
        response["success"] = true;
        response["data"] = doctorInfo;
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["error"] = QString("数据库操作失败: %1").arg(e.what());
        qDebug() << "getDoctorDetails error:" << e.what();
    }
    
    return response;
}

QJsonObject DoctorInfoModule::getDoctorSchedule(const QString& doctorUsername) {
    QJsonObject response;
    response["type"] = "doctorinfo_schedule_response";
    
    try {
        DBManager db(DatabaseConfig::getDatabasePath());
        
        QJsonArray schedules;
        if (!db.getDoctorSchedules(doctorUsername, schedules)) {
            response["success"] = false;
            response["error"] = "获取医生排班信息失败";
            return response;
        }
        
        response["success"] = true;
        response["data"] = schedules;
        
    } catch (const std::exception& e) {
        response["success"] = false;
        response["error"] = QString("数据库操作失败: %1").arg(e.what());
        qDebug() << "getDoctorSchedule error:" << e.what();
    }
    
    return response;
}

void DoctorInfoModule::onRequestReceived(const QJsonObject& payload) {
    const QString action = payload.value("action").toString();
    
    // 只处理医生信息相关的请求
    if (!action.startsWith("doctorinfo_")) {
        return;
    }
    
    QJsonObject response = handleDoctorInfoRequest(payload);
    
    // 添加 request_uuid 用于回复路由
    if (payload.contains("uuid")) {
        response["request_uuid"] = payload.value("uuid").toString();
    }
    Log::response("DoctorInfo", response);
    // 通过消息路由器回复
    MessageRouter::instance().onBusinessResponse(
        Protocol::MessageType::JsonResponse,
        response
    );
}
