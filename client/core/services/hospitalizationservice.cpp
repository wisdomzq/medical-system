#include "core/services/hospitalizationservice.h"
#include "core/network/communicationclient.h"

HospitalizationService::HospitalizationService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &HospitalizationService::onJsonReceived);
}

void HospitalizationService::fetchByPatient(const QString& patientUsername)
{
    QJsonObject req{{"action", "get_hospitalizations_by_patient"}, {"patient_username", patientUsername}};
    m_client->sendJson(req);
}

void HospitalizationService::fetchByDoctor(const QString& doctorUsername)
{
    QJsonObject req{{"action", "get_hospitalizations_by_doctor"}, {"doctor_username", doctorUsername}};
    m_client->sendJson(req);
}

void HospitalizationService::create(const QJsonObject& data)
{
    QJsonObject req{{"action", "create_hospitalization"}, {"data", data}};
    m_client->sendJson(req);
}

void HospitalizationService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "hospitalizations_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit fetched(obj.value("data").toArray());
        else emit fetchFailed(obj.value("error").toString());
    } else if (type == "create_hospitalization_response") {
        const bool ok = obj.value("success").toBool();
        emit created(ok, ok ? QString() : obj.value("error").toString());
    }
}
