#include "core/services/evaluateservice.h"
#include "core/network/communicationclient.h"
#include <QJsonObject>

EvaluateService::EvaluateService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &EvaluateService::onJsonReceived);
}

void EvaluateService::fetchConfig(const QString& patientUsername)
{
    QJsonObject req{{"action", "evaluate_get_config"}, {"patient_username", patientUsername}};
    m_client->sendJson(req);
}

void EvaluateService::recharge(const QString& patientUsername, double amount)
{
    QJsonObject req{{"action", "evaluate_recharge"}, {"patient_username", patientUsername}, {"amount", amount}};
    m_client->sendJson(req);
}

void EvaluateService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "evaluate_config_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit configReceived(obj.value("balance").toDouble());
        else emit configFailed(obj.value("error").toString());
        return;
    }
    if (type == "evaluate_recharge_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit rechargeSucceeded(obj.value("balance").toDouble(), obj.value("amount").toDouble());
        else emit rechargeFailed(obj.value("error").toString());
        return;
    }
}
