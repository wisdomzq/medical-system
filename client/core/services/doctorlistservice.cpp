#include "core/services/doctorlistservice.h"
#include "core/network/communicationclient.h"

DoctorListService::DoctorListService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &DoctorListService::onJsonReceived);
}

void DoctorListService::fetchAllDoctors()
{
    QJsonObject req{{"action", "get_all_doctors"}};
    m_client->sendJson(req);
}

void DoctorListService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "doctors_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit fetched(obj.value("data").toArray());
        else emit failed(obj.value("error").toString());
    }
}
