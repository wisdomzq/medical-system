#include "patientinfowidget.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

PatientInfoWidget::PatientInfoWidget(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("患者中心");
    setMinimumSize(800, 600);

    tabWidget = new QTabWidget(this);
    tabWidget->addTab(createAppointmentPage(), "我的预约");
    tabWidget->addTab(createCasePage(), "我的病例");
    tabWidget->addTab(createCommunicationPage(), "医患沟通");
    tabWidget->addTab(createProfilePage(), "个人信息");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
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

void PatientInfoWidget::setPatientName(const QString &patientName)
{
    currentPatientName = patientName;
    setWindowTitle("患者中心 - " + currentPatientName);
    loadProfile();
}

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

void PatientInfoWidget::loadProfile()
{
    DBManager db("user.db");
    int age;
    QString phone, address;
    if (db.getPatientDetails(currentPatientName, age, phone, address)) {
        nameEdit->setText(currentPatientName);
        ageEdit->setText(QString::number(age));
        phoneEdit->setText(phone);
        addressEdit->setText(address);
    }
}

void PatientInfoWidget::updateProfile()
{
    QString newName = nameEdit->text();
    int age = ageEdit->text().toInt();
    QString phone = phoneEdit->text();
    QString address = addressEdit->text();

    if (newName.isEmpty()) {
        QMessageBox::warning(this, "更新失败", "姓名不能为空！");
        return;
    }

    DBManager db("user.db");
    if (db.updatePatientProfile(currentPatientName, newName, age, phone, address)) {
        QMessageBox::information(this, "成功", "个人信息更新成功！");
        currentPatientName = newName;
        setWindowTitle("患者中心 - " + currentPatientName);
    } else {
        QMessageBox::warning(this, "失败", "个人信息更新失败！");
    }
}
