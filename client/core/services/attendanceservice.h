#ifndef ATTENDANCESERVICE_H
#define ATTENDANCESERVICE_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

// 医生日常考勤/请假服务
class AttendanceService : public QObject {
    Q_OBJECT
public:
    explicit AttendanceService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    // APIs
    void checkIn(const QString& doctorUsername, const QString& date, const QString& time);
    void submitLeave(const QString& doctorUsername, const QString& leaveDate, const QString& reason);
    void getActiveLeaves(const QString& doctorUsername);
    void cancelLeave(int leaveId);
    void getAttendanceHistory(const QString& doctorUsername);

signals:
    void checkInResult(bool success, const QString& message);
    void leaveSubmitted(bool success, const QString& message, const QJsonObject& data);
    void activeLeavesReceived(const QJsonArray& leaves);
    void attendanceHistoryReceived(const QJsonArray& rows);
    void cancelLeaveResult(bool success, const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client {nullptr}; // 非拥有
};

#endif // ATTENDANCESERVICE_H
