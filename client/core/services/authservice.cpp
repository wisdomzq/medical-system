#include "core/services/authservice.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/logging/logging.h"

AuthService::AuthService(QObject* parent)
    : QObject(parent)
{
    m_client = new CommunicationClient(this);
    connect(m_client, &CommunicationClient::connected, this, &AuthService::connected);
    connect(m_client, &CommunicationClient::disconnected, this, &AuthService::disconnected);
    connect(m_client, &CommunicationClient::errorOccurred, this, &AuthService::networkError);
    connect(m_client, &CommunicationClient::jsonReceived, this, &AuthService::onJsonReceived);
}

AuthService::AuthService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent)
{
    m_client = sharedClient;
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::connected, this, &AuthService::connected);
    connect(m_client, &CommunicationClient::disconnected, this, &AuthService::disconnected);
    connect(m_client, &CommunicationClient::errorOccurred, this, &AuthService::networkError);
    connect(m_client, &CommunicationClient::jsonReceived, this, &AuthService::onJsonReceived);
}

AuthService::~AuthService() = default;

void AuthService::connectDefault()
{
    m_client->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

void AuthService::login(const QString& username, const QString& password)
{
    m_lastLoginUsername = username;
    QJsonObject request{{"action", "login"}, {"username", username}, {"password", password}};
    Log::request("AuthService", request, "username", username);
    m_client->sendJson(request);
}

void AuthService::registerDoctor(const QString& username, const QString& password,
                                 const QString& department, const QString& phone)
{
    QJsonObject request{{"action", "register"}, {"role", "doctor"}, {"username", username},
                        {"password", password}, {"department", department}, {"phone", phone}};
    Log::request("AuthService", request, "role", "doctor", "username", username);
    m_client->sendJson(request);
}

void AuthService::registerPatient(const QString& username, const QString& password,
                                  int age, const QString& phone, const QString& address)
{
    QJsonObject request{{"action", "register"}, {"role", "patient"}, {"username", username},
                        {"password", password}, {"age", age}, {"phone", phone}, {"address", address}};
    Log::request("AuthService", request, "role", "patient", "username", username);
    m_client->sendJson(request);
}

void AuthService::onJsonReceived(const QJsonObject& response)
{
    const QString type = response.value("type").toString();
    const bool success = response.value("success").toBool(false);
    const QString message = response.value("message").toString();

    if (type == "login_response") {
        if (success) {
            const QString role = response.value("role").toString();
            emit loginSucceeded(role, m_lastLoginUsername, message);
        } else {
            emit loginFailed(message.isEmpty() ? QStringLiteral("登录失败") : message);
        }
    } else if (type == "register_response") {
        if (success) {
            const QString role = response.value("role").toString();
            emit registerSucceeded(role, message);
        } else {
            emit registerFailed(message.isEmpty() ? QStringLiteral("注册失败") : message);
        }
    }
}
