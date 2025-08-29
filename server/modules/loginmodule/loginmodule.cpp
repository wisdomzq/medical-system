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
        ok = m_db->registerDoctor(username, password, request.value("department").toString(), request.value("phone").toString());
        if (!ok) {
            errorMessage = "医生注册失败，用户名可能已存在或信息不完整。";
        }
    } else if (role == "patient") {
        ok = m_db->registerPatient(username, password, request.value("age").toInt(), request.value("phone").toString(), request.value("address").toString());
        if (!ok) {
            errorMessage = "病人注册失败，用户名可能已存在或信息不完整。";
        }
    } else {
        errorMessage = "无效的用户角色。";
    }
    
    response["success"] = ok;
    
    if (!ok) {
        response["message"] = errorMessage;
    } else {
        response["message"] = QString("%1注册成功！").arg(role == "doctor" ? "医生" : "病人");
    }

    return response;
}
