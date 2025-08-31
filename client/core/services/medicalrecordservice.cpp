#include "core/services/medicalrecordservice.h"
#include "core/network/communicationclient.h"

MedicalRecordService::MedicalRecordService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &MedicalRecordService::onJsonReceived);
}

void MedicalRecordService::fetchByPatient(const QString& patientUsername)
{
    QJsonObject req{{"action", "get_medical_records"}, {"patient_username", patientUsername}};
    m_client->sendJson(req);
}

void MedicalRecordService::fetchByDoctor(const QString& doctorUsername)
{
    QJsonObject req{{"action", "get_medical_records_by_doctor"}, {"doctor_username", doctorUsername}};
    m_client->sendJson(req);
}

void MedicalRecordService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "medical_records_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit fetched(obj.value("data").toArray());
        else emit fetchFailed(obj.value("error").toString());
    }
}
