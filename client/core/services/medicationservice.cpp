#include "core/services/medicationservice.h"
#include "core/network/communicationclient.h"
#include <QJsonObject>

MedicationService::MedicationService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &MedicationService::onJsonReceived);
}

void MedicationService::fetchAll()
{
    QJsonObject req{{"action", "get_medications"}};
    m_client->sendJson(req);
}

void MedicationService::search(const QString& keyword)
{
    if (keyword.isEmpty()) return fetchAll();
    QJsonObject req{{"action", "search_medications"}, {"keyword", keyword}};
    m_client->sendJson(req);
}

void MedicationService::searchRemote(const QString& keyword)
{
    if (keyword.isEmpty()) return; // 远端搜索要求关键词
    QJsonObject req{{"action", "search_medications_remote"}, {"keyword", keyword}};
    m_client->sendJson(req);
}

void MedicationService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "medications_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit medicationsFetched(obj.value("data").toArray());
        else emit fetchFailed(obj.value("error").toString());
    }
}
