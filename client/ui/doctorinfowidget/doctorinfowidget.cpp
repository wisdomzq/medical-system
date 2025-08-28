#include "doctorinfowidget.h"
#include "tcpclient.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QJsonObject>

DoctorInfoWidget::DoctorInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    m_tcpClient = new TcpClient(this);
    m_tcpClient->connectToServer("127.0.0.1", 12345);
    connect(m_tcpClient, &TcpClient::responseReceived, this, &DoctorInfoWidget::onResponseReceived);

    setWindowTitle("医生工作台");
    setMinimumSize(800, 600);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(createAppointmentPage(), "预约管理");
    tabWidget->addTab(createCasePage(), "病例管理");
    tabWidget->addTab(createCommunicationPage(), "医患沟通");
    tabWidget->addTab(createProfilePage(), "个人信息");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

DoctorInfoWidget::~DoctorInfoWidget()
{
}

QWidget* DoctorInfoWidget::createAppointmentPage()
{
    QWidget *page = new QWidget;
    // Add content for appointment management here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("预约管理模块"));
    return page;
}

QWidget* DoctorInfoWidget::createCasePage()
{
    QWidget *page = new QWidget;
    // Add content for case management here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("病例管理模块"));
    return page;
}

QWidget* DoctorInfoWidget::createCommunicationPage()
{
    QWidget *page = new QWidget;
    // Add content for communication here
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->addWidget(new QLabel("医患沟通模块"));
    return page;
}

void DoctorInfoWidget::setDoctorName(const QString &doctorName)
{
    currentDoctorName = doctorName;
    setWindowTitle("医生工作台 - " + currentDoctorName);
    loadProfile();
}

QWidget* DoctorInfoWidget::createProfilePage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    QFormLayout *formLayout = new QFormLayout;
    nameEdit = new QLineEdit;
    departmentEdit = new QLineEdit;
    phoneEdit = new QLineEdit;

    formLayout->addRow("姓名:", nameEdit);
    formLayout->addRow("科室:", departmentEdit);
    formLayout->addRow("电话:", phoneEdit);

    QPushButton *updateButton = new QPushButton("更新信息");
    connect(updateButton, &QPushButton::clicked, this, &DoctorInfoWidget::updateProfile);

    QPushButton *backButton = new QPushButton("返回登录");
    connect(backButton, &QPushButton::clicked, this, &DoctorInfoWidget::backToLogin);

    layout->addLayout(formLayout);
    layout->addWidget(updateButton);
    layout->addWidget(backButton);
    layout->addStretch();

    return page;
}

void DoctorInfoWidget::loadProfile()
{
    QJsonObject request;
    request["type"] = "get_doctor_info";
    request["username"] = currentDoctorName;
    m_tcpClient->sendRequest(request);
}

void DoctorInfoWidget::updateProfile()
{
    QJsonObject request;
    request["type"] = "update_doctor_info";
    request["username"] = currentDoctorName;

    QJsonObject data;
    data["name"] = nameEdit->text();
    data["department"] = departmentEdit->text();
    data["phone"] = phoneEdit->text();
    request["data"] = data;

    m_tcpClient->sendRequest(request);
}

void DoctorInfoWidget::onResponseReceived(const QJsonObject &response)
{
    QString type = response["type"].toString();
    if (type == "doctor_info_response" && response["success"].toBool())
    {
        QJsonObject data = response["data"].toObject();
        nameEdit->setText(data["name"].toString());
        departmentEdit->setText(data["department"].toString());
        phoneEdit->setText(data["phone"].toString());
    }
    else if (type == "update_doctor_info_response")
    {
        if (response["success"].toBool()) {
            QMessageBox::information(this, "成功", "个人信息更新成功！");
            currentDoctorName = nameEdit->text();
            setWindowTitle("医生工作台 - " + currentDoctorName);
        } else {
            QMessageBox::warning(this, "失败", "个人信息更新失败。");
        }
    }
}
