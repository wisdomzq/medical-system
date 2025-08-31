#pragma once
#include "basepage.h"
#include <QJsonObject>

class PatientAppointmentService;

class QTableWidget; class QPushButton; class QLineEdit;

class AppointmentPage : public BasePage {
    Q_OBJECT
public:
    explicit AppointmentPage(CommunicationClient *c, const QString &patient, QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj); // 将被废弃，迁移到服务层后不再使用

private slots:
    void requestDoctorSchedule();
    void requestAppointments();
    void sendRegisterRequest();

private:
    QTableWidget *doctorTable=nullptr; QTableWidget *appointmentsTable=nullptr;
    QPushButton *refreshDoctorsBtn=nullptr; QPushButton *registerBtn=nullptr; QPushButton *refreshAppointmentsBtn=nullptr;
    QLineEdit *doctorIdEdit=nullptr; QLineEdit *doctorNameEdit=nullptr; QLineEdit *patientNameEdit=nullptr;
    PatientAppointmentService* m_service = nullptr; // 非拥有
};
