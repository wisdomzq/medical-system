#include "profile.h"
#include "core/database/database.h"
#include "core/database/database_config.h"

QJsonObject DoctorProfileModule::handle(const QJsonObject& request) {
    const QString action = request.value("action").toString();
    if (action == "get_doctor_info") {
        return handleGet(request);
    } else if (action == "update_doctor_info") {
        return handleUpdate(request);
    }
    QJsonObject resp; resp["type"] = "unknown_response"; resp["success"] = false; resp["error"] = QString("Unknown action: %1").arg(action);
    return resp;
}

QJsonObject DoctorProfileModule::handleGet(const QJsonObject& request) {
    QJsonObject resp; resp["type"] = "doctor_info_response"; resp["success"] = false;
    const QString username = request.value("username").toString();
    if (username.isEmpty()) { resp["message"] = "username required"; return resp; }
    DBManager db(DatabaseConfig::getDatabasePath());
    QJsonObject data;
    if (db.getDoctorInfo(username, data)) {
        resp["success"] = true; resp["data"] = data;
    } else {
        // 未找到医生信息时，返回成功并给出可编辑的默认结构，便于前端填充后提交
        QJsonObject def;
        def["name"] = username;
        def["department"] = "";
        def["phone"] = "";
        def["email"] = "";
        def["work_number"] = "";
        def["title"] = ""; // 可由前端用 HH:mm-HH:mm 编码
        def["specialization"] = ""; // 个人资料
        def["consultation_fee"] = 0.0;
        def["max_patients_per_day"] = 0;
        resp["success"] = true;
        resp["data"] = def;
    }
    return resp;
}

QJsonObject DoctorProfileModule::handleUpdate(const QJsonObject& request) {
    QJsonObject resp; resp["type"] = "update_doctor_info_response"; resp["success"] = false;
    const QString username = request.value("username").toString();
    const QJsonObject data = request.value("data").toObject();
    if (username.isEmpty()) { resp["message"] = "username required"; return resp; }
    DBManager db(DatabaseConfig::getDatabasePath());
    // 可选：最小化兜底，保证缺失键有默认值，避免绑定异常
    QJsonObject patched = data;
    if (!patched.contains("name")) patched["name"] = username;
    if (!patched.contains("department")) patched["department"] = "";
    if (!patched.contains("phone")) patched["phone"] = QJsonValue();
    if (!patched.contains("email")) patched["email"] = QJsonValue();
    if (!patched.contains("work_number")) patched["work_number"] = "";
    if (!patched.contains("title")) patched["title"] = "";
    if (!patched.contains("specialization")) patched["specialization"] = "";
    if (!patched.contains("consultation_fee")) patched["consultation_fee"] = 0.0;
    if (!patched.contains("max_patients_per_day")) patched["max_patients_per_day"] = 0;

    if (db.updateDoctorInfo(username, patched)) {
        resp["success"] = true;
    } else {
        resp["message"] = QStringLiteral("update failed");
    }
    return resp;
}
