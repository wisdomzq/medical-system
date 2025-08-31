#include "core/services/patientappointmentservice.h"
#include "core/network/communicationclient.h"
#include <QDateTime>

PatientAppointmentService::PatientAppointmentService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient)
{
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &PatientAppointmentService::onJsonReceived);
}

void PatientAppointmentService::fetchAllDoctors()
{
    QJsonObject req{{"action", "get_all_doctors"}};
    m_client->sendJson(req);
}

void PatientAppointmentService::fetchAppointmentsForPatient(const QString& patientUsername)
{
    QJsonObject req{{"action", "get_appointments_by_patient"}, {"username", patientUsername}};
    m_client->sendJson(req);
}

void PatientAppointmentService::createAppointment(const QJsonObject& data, const QString& uuid)
{
    QJsonObject req{{"action", "create_appointment"}, {"data", data}};
    if (!uuid.isEmpty()) req.insert("uuid", uuid);
    else req.insert("uuid", QString("appt_req_%1").arg(QDateTime::currentMSecsSinceEpoch()));
    m_client->sendJson(req);
}

void PatientAppointmentService::onJsonReceived(const QJsonObject& obj)
{
    const auto type = obj.value("type").toString();
    if (type == "doctors_response" || type == "doctor_schedule_response" || type == "doctors_schedule_overview_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit doctorsFetched(obj.value("data").toArray());
        else emit doctorsFetchFailed(obj.value("error").toString());
        return;
    }

    if (type == "appointments_response") {
        const bool ok = obj.value("success").toBool();
        if (ok) emit appointmentsFetched(obj.value("data").toArray());
        else emit appointmentsFetchFailed(obj.value("error").toString());
        return;
    }

    if (type == "create_appointment_response" || type == "register_doctor_response") {
        const auto currentUuid = obj.value("request_uuid").toString();
        if (!currentUuid.isEmpty() && currentUuid == m_lastProcessedUuid) return; // 去重
        m_lastProcessedUuid = currentUuid;
        const bool ok = obj.value("success").toBool();
        if (ok) emit createSucceeded(obj.value("message").toString());
        else emit createFailed(obj.value("error").toString());
        return;
    }
}
