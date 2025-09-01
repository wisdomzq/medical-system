#include "core/services/doctorlistservice.h"
#include "core/network/communicationclient.h"
#include "core/logging/logging.h"

DoctorListService::DoctorListService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &DoctorListService::onJsonReceived);
}

void DoctorListService::fetchAllDoctors()
{
    QJsonObject req{{"action", "get_all_doctors"}};
    Log::request("DoctorListService", req);
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
