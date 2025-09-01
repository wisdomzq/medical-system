#include "core/services/appointmentservice.h"
#include "core/network/communicationclient.h"
#include "core/logging/logging.h"

AppointmentService::AppointmentService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &AppointmentService::onJsonReceived);
}

void AppointmentService::fetchByDoctor(const QString& doctorUsername)
{
    QJsonObject req{{"action", "get_appointments_by_doctor"}, {"username", doctorUsername}};
    Log::request("AppointmentService", req, "doctor", doctorUsername);
    m_client->sendJson(req);
}

void AppointmentService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "appointments_response") {
        const auto ok = obj.value("success").toBool();
        if (ok) emit fetched(obj.value("data").toArray());
        else emit fetchFailed(obj.value("error").toString());
    }
}
