#include "core/services/adviceservice.h"
#include "core/network/communicationclient.h"

AdviceService::AdviceService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &AdviceService::onJsonReceived);
}

void AdviceService::fetchAdviceList(const QString& patientUsername)
{
    QJsonObject req{{"action", "advice_get_list"}, {"patient_username", patientUsername}};
    m_client->sendJson(req);
}

void AdviceService::fetchAdviceDetails(int adviceId)
{
    QJsonObject req{{"action", "advice_get_details"}, {"advice_id", adviceId}};
    m_client->sendJson(req);
}

void AdviceService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "advice_list_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit listFetched(obj.value("data").toArray());
        else emit listFailed(obj.value("error").toString());
        return;
    }
    if (type == "advice_details_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit detailsFetched(obj.value("data").toObject());
        else emit detailsFailed(obj.value("error").toString());
        return;
    }
}
