#ifndef PATIENTINFOWIDGET_H
#define PATIENTINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>

class QTableWidget;
class QPushButton;
#include "core/network/src/client/communicationclient.h"

class QTabWidget;

class PatientInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PatientInfoWidget(const QString &patientName, QWidget *parent = nullptr);
    ~PatientInfoWidget();

private:
    QTabWidget *tabWidget;
    QString m_patientName;
    QLineEdit *nameEdit;
    QLineEdit *ageEdit;
    QLineEdit *phoneEdit;
    QLineEdit *addressEdit;
    CommunicationClient *m_communicationClient;

    QWidget *createAppointmentPage(); // 将扩展为医生排班 + 挂号
    QWidget *createCasePage();
    QWidget *createCommunicationPage();
    QWidget *createProfilePage();
    void requestPatientInfo();
    void requestDoctorSchedule();
    void sendRegisterRequest();

    // 挂号页面控件
    QWidget *appointmentPage = nullptr;
    QTableWidget *doctorTable = nullptr;
    QPushButton *refreshDoctorsBtn = nullptr;
    QPushButton *registerBtn = nullptr;
    QLineEdit *registerDoctorIdEdit = nullptr;
    QLineEdit *registerPatientNameEdit = nullptr;
    QTableWidget *appointmentsTable = nullptr; // 患者预约列表
    QPushButton *refreshAppointmentsBtn = nullptr;

private slots:
    void updateProfile();
    void onResponseReceived(const QJsonObject &response);

signals:
    void backToLogin();
};

#endif // PATIENTINFOWIDGET_H
