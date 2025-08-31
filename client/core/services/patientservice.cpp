#include "core/services/patientservice.h"
#include "core/network/communicationclient.h"

PatientService::PatientService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &PatientService::onJsonReceived);
}

void PatientService::requestPatientInfo(const QString& username)
{
    QJsonObject req; req["action"] = "get_patient_info"; req["username"] = username; m_client->sendJson(req);
}

void PatientService::updatePatientInfo(const QString& username, const QJsonObject& data)
{
    QJsonObject req; req["action"] = "update_patient_info"; req["username"] = username; req["data"] = data; m_client->sendJson(req);
}

void PatientService::onJsonReceived(const QJsonObject& obj)
{
    const QString type = obj.value("type").toString();
    if (type == "patient_info_response") {
        if (obj.value("success").toBool())
            emit patientInfoReceived(obj.value("data").toObject());
    } else if (type == "update_patient_info_response") {
        emit updatePatientInfoResult(obj.value("success").toBool(), obj.value("message").toString());
    }
}
