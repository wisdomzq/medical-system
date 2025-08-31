#include "core/services/doctorprofileservice.h"
#include "core/network/communicationclient.h"

DoctorProfileService::DoctorProfileService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &DoctorProfileService::onJsonReceived);
}

void DoctorProfileService::requestDoctorInfo(const QString& username)
{
    QJsonObject req; req["action"] = "get_doctor_info"; req["username"] = username; m_client->sendJson(req);
}

void DoctorProfileService::updateDoctorInfo(const QString& username, const QJsonObject& data)
{
    QJsonObject req; req["action"] = "update_doctor_info"; req["username"] = username; req["data"] = data; m_client->sendJson(req);
}

void DoctorProfileService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "doctor_info_response") {
        if (obj.value("success").toBool())
            emit infoReceived(obj.value("data").toObject());
        else
            emit infoFailed(obj.value("message").toString());
    } else if (type == "update_doctor_info_response") {
        emit updateResult(obj.value("success").toBool(), obj.value("message").toString());
    }
}
