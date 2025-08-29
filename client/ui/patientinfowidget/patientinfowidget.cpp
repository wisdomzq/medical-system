#include "patientinfowidget.h"
#include "core/network/src/protocol.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>

PatientInfoWidget::PatientInfoWidget(const QString &patientName, QWidget *parent)
    : QWidget(parent), m_patientName(patientName)
{
    m_communicationClient = new CommunicationClient(this);
    m_communicationClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
    connect(m_communicationClient, &CommunicationClient::jsonReceived, this, &PatientInfoWidget::onResponseReceived);

    setWindowTitle("患者中心 - " + m_patientName);
    setMinimumSize(800, 600);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(createAppointmentPage(), "我的预约");
    tabWidget->addTab(createCasePage(), "我的病例");
    tabWidget->addTab(createCommunicationPage(), "医患沟通");
    tabWidget->addTab(createProfilePage(), "个人信息");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);

    requestPatientInfo();
}

PatientInfoWidget::~PatientInfoWidget()
{
}

QWidget* PatientInfoWidget::createAppointmentPage()
{
    QWidget *page = new QWidget;
    // Add content for appointment management here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("我的预约模块"));
    return page;
}

QWidget* PatientInfoWidget::createCasePage()
{
    QWidget *page = new QWidget;
    // Add content for case management here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("我的病例模块"));
    return page;
}

QWidget* PatientInfoWidget::createCommunicationPage()
{
    QWidget *page = new QWidget;
    // Add content for communication here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("医患沟通模块"));
    return page;
}

// setPatientName removed; fixed via constructor

QWidget* PatientInfoWidget::createProfilePage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QFormLayout *formLayout = new QFormLayout;
    nameEdit = new QLineEdit;
    ageEdit = new QLineEdit;
    phoneEdit = new QLineEdit;
    addressEdit = new QLineEdit;

    formLayout->addRow("姓名:", nameEdit);
    formLayout->addRow("年龄:", ageEdit);
    formLayout->addRow("电话:", phoneEdit);
    formLayout->addRow("地址:", addressEdit);

    QPushButton *updateButton = new QPushButton("更新信息");
    connect(updateButton, &QPushButton::clicked, this, &PatientInfoWidget::updateProfile);

    QPushButton *backButton = new QPushButton("返回登录");
    connect(backButton, &QPushButton::clicked, this, &PatientInfoWidget::backToLogin);

    layout->addLayout(formLayout);
    layout->addWidget(updateButton);
    layout->addWidget(backButton);
    layout->addStretch();

    return page;
}

void PatientInfoWidget::requestPatientInfo()
{
    QJsonObject request;
    request["action"] = "get_patient_info";
    request["username"] = m_patientName;
    m_communicationClient->sendJson(request);
}

void PatientInfoWidget::updateProfile()
{
    // This would be a new request type, e.g., "update_patient_info"
    QMessageBox::information(this, "提示", "更新功能需要服务器端实现。");
}

void PatientInfoWidget::onResponseReceived(const QJsonObject &response)
{
    QString type = response["type"].toString();
    if (type == "patient_info_response" && response["success"].toBool())
    {
        QJsonObject data = response["data"].toObject();
        nameEdit->setText(data["name"].toString());
        ageEdit->setText(QString::number(data["age"].toInt()));
        phoneEdit->setText(data["phone"].toString());
        addressEdit->setText(data["address"].toString());
    }
}
