#include "loginmodule.h"
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>
#include "core/database/database.h"
#include "core/database/database_config.h"

LoginModule::LoginModule(QObject *parent) : QObject(parent), m_db(new DBManager(DatabaseConfig::getDatabasePath())) {}

LoginModule::~LoginModule() { delete m_db; }

QJsonObject LoginModule::handleLogin(const QJsonObject &request)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();

    QJsonObject response;
    response["type"] = "login_response";
    bool ok = m_db->authenticateUser(username, password);
    response["success"] = ok;
    if (ok) {
        QString role;
        if (m_db->getUserRole(username, role)) {
            response["role"] = role;
        }
    }

    return response;
}

// ...existing code...
QJsonObject LoginModule::handleRegister(const QJsonObject &request)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();
    QString role = request["role"].toString();

    QJsonObject response;
    response["type"] = "register_response";
    bool ok = false;
    QString errorMessage = "";
    
    if (role == "doctor") {
        QString department = request.value("department").toString();
        QString phone = request.value("phone").toString();
        
        // 检查必需字段
        if (username.isEmpty() || password.isEmpty() || department.isEmpty() || phone.isEmpty()) {
            errorMessage = "医生注册失败，所有字段都必须填写。";
        } else {
            ok = m_db->registerDoctor(username, password, department, phone);
            if (!ok) {
                errorMessage = "医生注册失败，用户名可能已存在或数据库错误。";
            }
        }
    } else if (role == "patient") {
        int age = request.value("age").toInt();
        QString phone = request.value("phone").toString();
        QString address = request.value("address").toString();
        
        // 检查必需字段
        if (username.isEmpty() || password.isEmpty() || age <= 0 || phone.isEmpty() || address.isEmpty()) {
            errorMessage = "病人注册失败，所有字段都必须正确填写（年龄必须大于0）。";
        } else {
            ok = m_db->registerPatient(username, password, age, phone, address);
            if (!ok) {
                errorMessage = "病人注册失败，用户名可能已存在或数据库错误。";
            }
        }
    } else {
        errorMessage = "注册失败，无效的用户角色。";
    }
    
    response["success"] = ok;
    if (ok) {
        response["message"] = "注册成功！";
    } else {
        response["message"] = errorMessage;
    }

    return response;
}
