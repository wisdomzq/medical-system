#include "patientinfowidget.h"
#include "core/network/src/protocol.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
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

QWidget* PatientInfoWidget::createAppointmentPage() {
    appointmentPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(appointmentPage);

    // 操作区
    QHBoxLayout *toolbar = new QHBoxLayout;
    refreshDoctorsBtn = new QPushButton("刷新医生排班");
    registerBtn = new QPushButton("挂号");
    registerDoctorIdEdit = new QLineEdit; registerDoctorIdEdit->setPlaceholderText("医生ID");
    registerPatientNameEdit = new QLineEdit; registerPatientNameEdit->setPlaceholderText("患者姓名(默认本人)");
    toolbar->addWidget(refreshDoctorsBtn);
    toolbar->addWidget(new QLabel("医生ID:")); toolbar->addWidget(registerDoctorIdEdit);
    toolbar->addWidget(new QLabel("患者:")); toolbar->addWidget(registerPatientNameEdit);
    toolbar->addWidget(registerBtn);
    toolbar->addStretch();

    doctorTable = new QTableWidget; // 列: ID, 姓名, 科室, 工作时间, 费用, 已预约/上限, 剩余
    doctorTable->setColumnCount(7);
    QStringList headers{"ID","姓名","科室","工作时间","费用","已/上限","剩余"};
    doctorTable->setHorizontalHeaderLabels(headers);
    doctorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addLayout(toolbar);
    layout->addWidget(new QLabel("医生排班"));
    layout->addWidget(doctorTable);

    // 患者预约列表部分
    QHBoxLayout *apptBar = new QHBoxLayout;
    refreshAppointmentsBtn = new QPushButton("刷新我的预约");
    apptBar->addWidget(refreshAppointmentsBtn);
    apptBar->addStretch();
    layout->addSpacing(12);
    layout->addLayout(apptBar);

    appointmentsTable = new QTableWidget; // 列: ID, 医生账号, 医生姓名, 日期, 时间, 科室, 状态, 费用
    appointmentsTable->setColumnCount(8);
    appointmentsTable->setHorizontalHeaderLabels({"ID","医生账号","医生姓名","日期","时间","科室","状态","费用"});
    appointmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    appointmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    appointmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(new QLabel("我的预约列表"));
    layout->addWidget(appointmentsTable);

    connect(refreshDoctorsBtn, &QPushButton::clicked, this, &PatientInfoWidget::requestDoctorSchedule);
    connect(refreshAppointmentsBtn, &QPushButton::clicked, this, [this]{
        QJsonObject req; req["action"] = "get_appointments_by_patient"; req["username"] = m_patientName; m_communicationClient->sendJson(req);
    });
    connect(registerBtn, &QPushButton::clicked, this, &PatientInfoWidget::sendRegisterRequest);
    connect(doctorTable, &QTableWidget::cellClicked, this, [this](int row,int){
        if (!doctorTable) return;
        registerDoctorIdEdit->setText(doctorTable->item(row,0)->text());
    });

    // 初次进入自动刷新
    QTimer::singleShot(300, this, &PatientInfoWidget::requestDoctorSchedule);
    QTimer::singleShot(600, this, [this]{ QJsonObject req; req["action"] = "get_appointments_by_patient"; req["username"] = m_patientName; m_communicationClient->sendJson(req); });
    return appointmentPage;
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

void PatientInfoWidget::requestDoctorSchedule() {
    QJsonObject req; req["action"] = "get_doctor_schedule"; m_communicationClient->sendJson(req);
}

void PatientInfoWidget::sendRegisterRequest() {
    int doctorId = registerDoctorIdEdit->text().toInt();
    if (doctorId <= 0) { QMessageBox::warning(this, "提示", "请输入有效的医生ID"); return; }
    QString patient = registerPatientNameEdit->text().trimmed();
    if (patient.isEmpty()) patient = m_patientName; // 默认本人
    QJsonObject req; req["action"] = "register_doctor"; req["doctorId"] = doctorId; req["patientName"] = patient;
    m_communicationClient->sendJson(req);
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
    else if (type == "doctor_schedule_response") {
        if (!doctorTable) return;
        doctorTable->setRowCount(0);
        if (!response["success"].toBool()) return;
        QJsonArray arr = response.value("data").toArray();
        doctorTable->setRowCount(arr.size());
        int row=0; for (auto v: arr) {
            QJsonObject o = v.toObject();
            auto setItem=[&](int col,const QString &text){
                auto *it=new QTableWidgetItem(text); doctorTable->setItem(row,col,it);
            };
            setItem(0, QString::number(o.value("doctorId").toInt()));
            setItem(1, o.value("name").toString());
            setItem(2, o.value("department").toString());
            setItem(3, o.value("workTime").toString());
            setItem(4, QString::number(o.value("fee").toDouble(),'f',2));
            int reserved = o.value("reservedPatients").toInt();
            int maxp = o.value("maxPatientsPerDay").toInt();
            int remain = o.value("remainingSlots").toInt();
            setItem(5, QString("%1/%2").arg(reserved).arg(maxp));
            setItem(6, QString::number(remain));
            ++row;
        }
    }
    else if (type == "register_doctor_response") {
        if (response["success"].toBool()) {
            QMessageBox::information(this, "成功", "挂号成功");
            requestDoctorSchedule();
        } else {
            QMessageBox::warning(this, "失败", response.value("error").toString());
        }
    }
    else if (type == "appointments_response") {
        if (!appointmentsTable) return;
        if (!response["success"].toBool()) return;
        QJsonArray arr = response.value("data").toArray();
        appointmentsTable->setRowCount(arr.size());
        int r=0; for (auto v: arr) {
            QJsonObject o=v.toObject();
            auto set=[&](int c,const QString&t){appointmentsTable->setItem(r,c,new QTableWidgetItem(t));};
            set(0, QString::number(o.value("id").toInt()));
            set(1, o.value("doctor_username").toString());
            set(2, o.value("doctor_name").toString());
            set(3, o.value("appointment_date").toString());
            set(4, o.value("appointment_time").toString());
            set(5, o.value("department").toString());
            set(6, o.value("status").toString());
            set(7, QString::number(o.value("fee").toDouble(),'f',2));
            ++r;
        }
    }
}
