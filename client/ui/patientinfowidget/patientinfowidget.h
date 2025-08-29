#ifndef PATIENTINFOWIDGET_H
#define PATIENTINFOWIDGET_H

#include <QWidget>
#include <QObject>
class QTabWidget;
class CommunicationClient;

// 前向声明各子页面类
class AppointmentPage;
class ProfilePage;
class CasePage;
class CommunicationPage;
class DoctorInfoPage;
class AdvicePage;
class PrescriptionPage;
class HealthAssessmentPage;
class MedicationSearchPage;
class HospitalPage;

class PatientInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PatientInfoWidget(const QString &patientName, QWidget *parent = nullptr);
    ~PatientInfoWidget();

private:
    QTabWidget *tabWidget = nullptr;
    QString m_patientName;
    CommunicationClient *m_communicationClient = nullptr;

    // 子页面实例
    AppointmentPage *m_appointmentPage = nullptr;
    ProfilePage *m_profilePage = nullptr;
    CasePage *m_casePage = nullptr;
    CommunicationPage *m_communicationPage = nullptr;
    DoctorInfoPage *m_doctorInfoPage = nullptr;
    AdvicePage *m_advicePage = nullptr;
    PrescriptionPage *m_prescriptionPage = nullptr;
    HealthAssessmentPage *m_healthAssessmentPage = nullptr;
    MedicationSearchPage *m_medicationSearchPage = nullptr;
    HospitalPage *m_hospitalPage = nullptr; // 新住院页面

private slots:
    void forwardBackToLogin();

signals:
    void backToLogin();
};

#endif // PATIENTINFOWIDGET_H
