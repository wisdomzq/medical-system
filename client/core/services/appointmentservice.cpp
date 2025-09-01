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
        return;
    }
    if (type == "update_appointment_status_response") {
        const bool ok = obj.value("success").toBool();
        QJsonObject data = obj.value("data").toObject(); // 可能无 data
        int apptId = data.value("appointment_id").toInt();
        QString status = data.value("status").toString();
        // 若服务端未回传 data，则无法得知 id/status，但我们依旧发射信号，调用处可自持状态
        emit statusUpdated(ok, apptId, status, obj.value("error").toString());
        return;
    }
}

void AppointmentService::updateStatus(int appointmentId, const QString& status)
{
    QJsonObject req;
    req["action"] = "update_appointment_status";
    QJsonObject data; data["appointment_id"] = appointmentId; data["status"] = status;
    req["data"] = data;
    Log::request("AppointmentService", req, "updateStatus", QString::number(appointmentId));
    m_client->sendJson(req);
}
