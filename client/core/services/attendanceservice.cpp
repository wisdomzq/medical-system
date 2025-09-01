#include "core/services/attendanceservice.h"
#include "core/network/communicationclient.h"
#include "core/logging/logging.h"

AttendanceService::AttendanceService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient) {
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &AttendanceService::onJsonReceived);
}

void AttendanceService::checkIn(const QString& doctorUsername, const QString& date, const QString& time) {
    QJsonObject req{{"action","doctor_checkin"}, {"doctor_username", doctorUsername}, {"checkin_date", date}, {"checkin_time", time}};
    Log::request("AttendanceService", req, "doctor", doctorUsername, "date", date, "time", time);
    m_client->sendJson(req);
}

void AttendanceService::submitLeave(const QString& doctorUsername, const QString& leaveDate, const QString& reason) {
    QJsonObject req{{"action","doctor_leave"}, {"doctor_username", doctorUsername}, {"leave_date", leaveDate}, {"reason", reason}};
    Log::request("AttendanceService", req, "doctor", doctorUsername, "date", leaveDate);
    m_client->sendJson(req);
}

void AttendanceService::getActiveLeaves(const QString& doctorUsername) {
    QJsonObject req{{"action","get_active_leaves"}, {"doctor_username", doctorUsername}};
    Log::request("AttendanceService", req, "doctor", doctorUsername);
    m_client->sendJson(req);
}

void AttendanceService::cancelLeave(int leaveId) {
    QJsonObject req{{"action","cancel_leave"}, {"leave_id", leaveId}};
    Log::request("AttendanceService", req, "leave_id", QString::number(leaveId));
    m_client->sendJson(req);
}

void AttendanceService::getAttendanceHistory(const QString& doctorUsername) {
    QJsonObject req{{"action","get_attendance_history"}, {"doctor_username", doctorUsername}};
    Log::request("AttendanceService", req, "doctor", doctorUsername);
    m_client->sendJson(req);
}

void AttendanceService::onJsonReceived(const QJsonObject& obj) {
    const QString type = obj.value("type").toString();
    if (type == "doctor_checkin_response") {
        emit checkInResult(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "doctor_leave_response") {
        emit leaveSubmitted(obj.value("success").toBool(), obj.value("message").toString(), obj.value("data").toObject());
        return;
    }
    if (type == "active_leaves_response") {
        if (obj.value("success").toBool()) emit activeLeavesReceived(obj.value("data").toArray());
        else emit activeLeavesReceived(QJsonArray());
        return;
    }
    if (type == "attendance_history_response") {
        if (obj.value("success").toBool()) emit attendanceHistoryReceived(obj.value("data").toArray());
        else emit attendanceHistoryReceived(QJsonArray());
        return;
    }
    if (type == "cancel_leave_response") {
        emit cancelLeaveResult(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
}
