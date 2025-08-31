#include "core/services/prescriptionservice.h"
#include "core/network/communicationclient.h"

PrescriptionService::PrescriptionService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &PrescriptionService::onJsonReceived);
}

void PrescriptionService::fetchList(const QString& patientUsername)
{
    QJsonObject req{{"action", "prescription_get_list"}, {"patient_username", patientUsername}};
    m_client->sendJson(req);
}

void PrescriptionService::fetchDetails(int prescriptionId)
{
    QJsonObject req{{"action", "prescription_get_details"}, {"prescription_id", prescriptionId}};
    m_client->sendJson(req);
}

void PrescriptionService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "prescription_list_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit listFetched(obj.value("data").toArray());
        else emit listFailed(obj.value("error").toString());
        return;
    }
    if (type == "prescription_details_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit detailsFetched(obj.value("data").toObject());
        else emit detailsFailed(obj.value("error").toString());
        return;
    }
}
