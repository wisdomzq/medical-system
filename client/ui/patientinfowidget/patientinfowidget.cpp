#include "patientinfowidget.h"
#include "core/network/src/protocol.h"
#include "core/network/src/client/communicationclient.h"
#include "appointmentpage.h"
#include "profilepage.h"
#include "casepage.h"
#include "placeholderpages.h"
#include "hospitalpage.h"
#include "medicationpage.h"
#include "evaluatepage.h"
#include "prescriptionpage.h"
#include "advicepage.h"
#include "doctorlistpage.h"
#include <QVBoxLayout>
#include <QTabWidget>
#include <QJsonObject>

PatientInfoWidget::PatientInfoWidget(const QString &patientName, QWidget *parent)
    : QWidget(parent), m_patientName(patientName)
{
    m_communicationClient = new CommunicationClient(this);
    m_communicationClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
    setWindowTitle("患者中心 - " + m_patientName);
    setMinimumSize(900, 650);

    tabWidget = new QTabWidget(this);
    m_appointmentPage = new AppointmentPage(m_communicationClient, m_patientName, this);
    m_casePage = new CasePage(m_communicationClient, m_patientName, this);
    m_communicationPage = new CommunicationPage(m_communicationClient, m_patientName, this);
    m_profilePage = new ProfilePage(m_communicationClient, m_patientName, this);
    connect(m_profilePage, &ProfilePage::backToLogin, this, &PatientInfoWidget::forwardBackToLogin);
    m_doctorInfoPage = new DoctorListPage(m_communicationClient, m_patientName, this);
    m_advicePage = new AdvicePage(m_communicationClient, m_patientName, this);
    m_prescriptionPage = new PrescriptionPage(m_communicationClient, m_patientName, this);
    
    // 连接医嘱页面到处方页面的跳转
    connect(m_advicePage, &AdvicePage::prescriptionRequested, 
            this, &PatientInfoWidget::switchToPrescriptionTab);
    
    m_evaluatePage = new EvaluatePage(m_communicationClient, m_patientName, this);
    m_medicationSearchPage = new MedicationSearchPage(m_communicationClient, m_patientName, this);
    m_hospitalPage = new HospitalPage(m_communicationClient, m_patientName, this); // 新实际页面

    tabWidget->addTab(m_appointmentPage, "我的预约");
    tabWidget->addTab(m_casePage, "我的病例");
    tabWidget->addTab(m_communicationPage, "医患沟通");
    tabWidget->addTab(m_profilePage, "个人信息");
    tabWidget->addTab(m_doctorInfoPage, "查看医生信息");
    tabWidget->addTab(m_advicePage, "医嘱");
    tabWidget->addTab(m_prescriptionPage, "处方");
    tabWidget->addTab(m_evaluatePage, "评估与充值");
    tabWidget->addTab(m_medicationSearchPage, "药品搜索");
    tabWidget->addTab(m_hospitalPage, "住院信息");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(tabWidget);
    setLayout(layout);

    connect(m_communicationClient, &CommunicationClient::jsonReceived, this, [this](const QJsonObject &obj){
        const QString type = obj.value("type").toString();
        if (type.startsWith("doctor_") || type == "register_doctor_response" || type == "appointments_response")
            m_appointmentPage->handleResponse(obj);
        if (type.startsWith("patient_info"))
            m_profilePage->handleResponse(obj);
        if (type.startsWith("hospitalizations"))
            m_hospitalPage->handleResponse(obj);
        if (type == "medications_response")
            m_medicationSearchPage->handleResponse(obj);
        if (type.startsWith("evaluate_"))
            m_evaluatePage->handleResponse(obj);
    });
}

PatientInfoWidget::~PatientInfoWidget() = default;

void PatientInfoWidget::forwardBackToLogin() { emit backToLogin(); }

void PatientInfoWidget::switchToPrescriptionTab(int prescriptionId) {
    // 切换到处方tab页面
    int prescriptionTabIndex = -1;
    for (int i = 0; i < tabWidget->count(); ++i) {
        if (tabWidget->widget(i) == m_prescriptionPage) {
            prescriptionTabIndex = i;
            break;
        }
    }
    
    if (prescriptionTabIndex >= 0) {
        tabWidget->setCurrentIndex(prescriptionTabIndex);
        // TODO: 如果需要，可以通知处方页面显示特定的处方
        Q_UNUSED(prescriptionId)
    }
}
