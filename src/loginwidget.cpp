#include "loginwidget.h"
#include "doctorinfowidget.h"
#include "patientinfowidget.h"
#include "tcpclient.h"
#include <QFont>
#include <QMessageBox>
#include <QJsonObject>

LoginWidget::LoginWidget(QWidget* parent)
    : QWidget(parent)
{
    m_tcpClient = new TcpClient(this);
    connect(m_tcpClient, &TcpClient::connected, this, [this](){
        QMessageBox::information(this, "连接成功", "已成功连接到服务器。");
    });
    connect(m_tcpClient, &TcpClient::disconnected, this, [this](){
        QMessageBox::warning(this, "连接断开", "与服务器的连接已断开。");
    });
    m_tcpClient->connectToServer("127.0.0.1", 12345);
    connect(m_tcpClient, &TcpClient::responseReceived, this, &LoginWidget::onResponseReceived);

    // Main layout containing only the stacked widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget);

    // --- Page 0: Initial Role Selection Page ---
    QWidget* selectionPage = new QWidget(this);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionPage);

    QLabel* userTypeLabel = new QLabel("请选择您的身份：", this);
    QFont userTypeFont;
    userTypeFont.setPointSize(16);
    userTypeLabel->setFont(userTypeFont);
    userTypeLabel->setAlignment(Qt::AlignCenter);

    QPushButton* selectDoctorBtn = new QPushButton("医生", this);
    QPushButton* selectPatientBtn = new QPushButton("病人", this);
    selectDoctorBtn->setFixedSize(120, 40);
    selectPatientBtn->setFixedSize(120, 40);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(selectDoctorBtn);
    btnLayout->addSpacing(40);
    btnLayout->addWidget(selectPatientBtn);
    btnLayout->addStretch();

    selectionLayout->addStretch();
    selectionLayout->addWidget(createTitleLabel()); // Use helper for title
    selectionLayout->addSpacing(20);
    selectionLayout->addWidget(userTypeLabel);
    selectionLayout->addSpacing(20);
    selectionLayout->addLayout(btnLayout);
    selectionLayout->addStretch();


    // --- Page 1: Doctor Login Page ---
    QWidget* doctorLoginPage = new QWidget(this);
    QVBoxLayout* doctorLoginLayout = new QVBoxLayout(doctorLoginPage);
    doctorLoginNameEdit = new QLineEdit(this);
    doctorLoginPasswordEdit = new QLineEdit(this);
    doctorLoginPasswordEdit->setEchoMode(QLineEdit::Password);
    doctorLoginButton = new QPushButton("登录", this);
    goToDoctorRegisterButton = new QPushButton("没有账户？点击注册", this);
    backFromDoctorLoginButton = new QPushButton("返回", this); // <-- 创建按钮

    QFormLayout* doctorFormLayout = new QFormLayout();
    doctorFormLayout->addRow("医生姓名：", doctorLoginNameEdit);
    doctorFormLayout->addRow("密码：", doctorLoginPasswordEdit);

    doctorLoginLayout->addStretch();
    doctorLoginLayout->addWidget(createTitleLabel());
    doctorLoginLayout->addSpacing(20);
    doctorLoginLayout->addLayout(doctorFormLayout);
    doctorLoginLayout->addSpacing(10);
    doctorLoginLayout->addWidget(doctorLoginButton);
    doctorLoginLayout->addWidget(goToDoctorRegisterButton);
    doctorLoginLayout->addWidget(backFromDoctorLoginButton); // <-- 添加到布局
    doctorLoginLayout->addStretch();


    // --- Page 2: Patient Login Page ---
    QWidget* patientLoginPage = new QWidget(this);
    QVBoxLayout* patientLoginLayout = new QVBoxLayout(patientLoginPage);
    patientLoginNameEdit = new QLineEdit(this);
    patientLoginPasswordEdit = new QLineEdit(this);
    patientLoginPasswordEdit->setEchoMode(QLineEdit::Password);
    patientLoginButton = new QPushButton("登录", this);
    goToPatientRegisterButton = new QPushButton("没有账户？点击注册", this);
    backFromPatientLoginButton = new QPushButton("返回", this); // <-- 创建按钮

    QFormLayout* patientFormLayout = new QFormLayout();
    patientFormLayout->addRow("病人姓名：", patientLoginNameEdit);
    patientFormLayout->addRow("密码：", patientLoginPasswordEdit);

    patientLoginLayout->addStretch();
    patientLoginLayout->addWidget(createTitleLabel());
    patientLoginLayout->addSpacing(20);
    patientLoginLayout->addLayout(patientFormLayout);
    patientLoginLayout->addSpacing(10);
    patientLoginLayout->addWidget(patientLoginButton);
    patientLoginLayout->addWidget(goToPatientRegisterButton);
    patientLoginLayout->addWidget(backFromPatientLoginButton); // <-- 添加到布局
    patientLoginLayout->addStretch();


    // --- Page 3: Doctor Registration Page ---
    QWidget* doctorRegisterPage = new QWidget(this);
    QVBoxLayout* doctorRegisterLayout = new QVBoxLayout(doctorRegisterPage);
    doctorRegisterNameEdit = new QLineEdit(this);
    doctorRegisterPasswordEdit = new QLineEdit(this);
    doctorRegisterPasswordEdit->setEchoMode(QLineEdit::Password);
    doctorDepartmentEdit = new QLineEdit(this);
    doctorPhoneEdit = new QLineEdit(this);
    doctorRegisterButton = new QPushButton("注册", this);
    backToDoctorLoginButton = new QPushButton("返回登录", this);
    
    QFormLayout* doctorRegisterForm = new QFormLayout();
    doctorRegisterForm->addRow("医生姓名：", doctorRegisterNameEdit);
    doctorRegisterForm->addRow("密码：", doctorRegisterPasswordEdit);
    doctorRegisterForm->addRow("科室：", doctorDepartmentEdit);
    doctorRegisterForm->addRow("电话：", doctorPhoneEdit);

    doctorRegisterLayout->addStretch();
    doctorRegisterLayout->addWidget(createTitleLabel());
    doctorRegisterLayout->addLayout(doctorRegisterForm);
    doctorRegisterLayout->addWidget(doctorRegisterButton);
    doctorRegisterLayout->addWidget(backToDoctorLoginButton);
    doctorRegisterLayout->addStretch();


    // --- Page 4: Patient Registration Page ---
    QWidget* patientRegisterPage = new QWidget(this);
    QVBoxLayout* patientRegisterLayout = new QVBoxLayout(patientRegisterPage);
    patientRegisterNameEdit = new QLineEdit(this);
    patientRegisterPasswordEdit = new QLineEdit(this);
    patientRegisterPasswordEdit->setEchoMode(QLineEdit::Password);
    patientAgeEdit = new QLineEdit(this);
    patientPhoneEdit = new QLineEdit(this);
    patientAddressEdit = new QLineEdit(this);
    patientRegisterButton = new QPushButton("注册", this);
    backToPatientLoginButton = new QPushButton("返回登录", this);

    QFormLayout* patientRegisterForm = new QFormLayout();
    patientRegisterForm->addRow("病人姓名：", patientRegisterNameEdit);
    patientRegisterForm->addRow("密码：", patientRegisterPasswordEdit);
    patientRegisterForm->addRow("年龄：", patientAgeEdit);
    patientRegisterForm->addRow("电话：", patientPhoneEdit);
    patientRegisterForm->addRow("地址：", patientAddressEdit);
    
    patientRegisterLayout->addStretch();
    patientRegisterLayout->addWidget(createTitleLabel());
    patientRegisterLayout->addLayout(patientRegisterForm);
    patientRegisterLayout->addWidget(patientRegisterButton);
    patientRegisterLayout->addWidget(backToPatientLoginButton);
    patientRegisterLayout->addStretch();

    // --- Add all pages to the stacked widget ---
    stackedWidget->addWidget(selectionPage);          // Index 0
    stackedWidget->addWidget(doctorLoginPage);        // Index 1
    stackedWidget->addWidget(patientLoginPage);       // Index 2
    stackedWidget->addWidget(doctorRegisterPage);     // Index 3
    stackedWidget->addWidget(patientRegisterPage);    // Index 4

    // --- Connect signals and slots ---
    // Navigation
    connect(selectDoctorBtn, &QPushButton::clicked, this, &LoginWidget::showDoctorLogin);
    connect(selectPatientBtn, &QPushButton::clicked, this, &LoginWidget::showPatientLogin);
    connect(goToDoctorRegisterButton, &QPushButton::clicked, this, &LoginWidget::showDoctorRegister);
    connect(goToPatientRegisterButton, &QPushButton::clicked, this, &LoginWidget::showPatientRegister);
    connect(backToDoctorLoginButton, &QPushButton::clicked, this, &LoginWidget::showDoctorLogin);
    connect(backToPatientLoginButton, &QPushButton::clicked, this, &LoginWidget::showPatientLogin);
    connect(backFromDoctorLoginButton, &QPushButton::clicked, this, &LoginWidget::showInitialSelection); // <-- 连接信号
    connect(backFromPatientLoginButton, &QPushButton::clicked, this, &LoginWidget::showInitialSelection); // <-- 连接信号

    // Actions
    connect(doctorLoginButton, &QPushButton::clicked, this, &LoginWidget::onDoctorLogin);
    connect(patientLoginButton, &QPushButton::clicked, this, &LoginWidget::onPatientLogin);
    connect(doctorRegisterButton, &QPushButton::clicked, this, &LoginWidget::onDoctorRegister);
    connect(patientRegisterButton, &QPushButton::clicked, this, &LoginWidget::onPatientRegister);
}

LoginWidget::~LoginWidget() {}

QLabel* LoginWidget::createTitleLabel() {
    QLabel* titleLabel = new QLabel("智慧医疗系统");
    QFont titleFont;
    titleFont.setPointSize(32);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("color: #1976d2; letter-spacing: 4px; padding: 24px 0; background: transparent;");
    return titleLabel;
}

// --- Slot Implementations ---

// ==================== 修改开始 ====================
// 添加缺失的函数实现
void LoginWidget::showInitialSelection() { stackedWidget->setCurrentIndex(0); }
// ==================== 修改结束 ====================

void LoginWidget::showDoctorLogin() { stackedWidget->setCurrentIndex(1); }
void LoginWidget::showPatientLogin() { stackedWidget->setCurrentIndex(2); }
void LoginWidget::showDoctorRegister() { stackedWidget->setCurrentIndex(3); }
void LoginWidget::showPatientRegister() { stackedWidget->setCurrentIndex(4); }

void LoginWidget::onDoctorLogin() {
    QString name = doctorLoginNameEdit->text();
    QString password = doctorLoginPasswordEdit->text();

    if (name.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空！");
        return;
    }

    QJsonObject request;
    request["type"] = "login";
    request["username"] = name;
    request["password"] = password;
    request["role"] = "doctor";
    m_tcpClient->sendRequest(request);
}

void LoginWidget::onPatientLogin() {
    QString name = patientLoginNameEdit->text();
    QString password = patientLoginPasswordEdit->text();

    if (name.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空！");
        return;
    }

    QJsonObject request;
    request["type"] = "login";
    request["username"] = name;
    request["password"] = password;
    request["role"] = "patient";
    m_tcpClient->sendRequest(request);
}

void LoginWidget::onDoctorRegister() {
    // 直接从输入框获取 QString
    QString name = doctorRegisterNameEdit->text();
    QString password = doctorRegisterPasswordEdit->text();
    QString department = doctorDepartmentEdit->text();
    QString phone = doctorPhoneEdit->text();

    // 添加输入验证
    if (name.isEmpty() || password.isEmpty() || department.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "所有注册信息均不能为空！");
        return;
    }

    QJsonObject request;
    request["type"] = "register";
    request["username"] = name;
    request["password"] = password;
    request["role"] = "doctor";
    request["department"] = department;
    request["phone"] = phone;
    m_tcpClient->sendRequest(request);
}

void LoginWidget::onPatientRegister() {
    // 直接从输入框获取 QString
    QString name = patientRegisterNameEdit->text();
    QString password = patientRegisterPasswordEdit->text();
    QString ageStr = patientAgeEdit->text();
    QString phone = patientPhoneEdit->text();
    QString address = patientAddressEdit->text();

    // 添加输入验证
    if (name.isEmpty() || password.isEmpty() || ageStr.isEmpty() || phone.isEmpty() || address.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "所有注册信息均不能为空！");
        return;
    }

    bool isAgeValid;
    int age = ageStr.toInt(&isAgeValid);

    if (!isAgeValid || age <= 0) {
        QMessageBox::warning(this, "注册失败", "请输入一个有效的年龄！");
        return;
    }

    QJsonObject request;
    request["type"] = "register";
    request["username"] = name;
    request["password"] = password;
    request["role"] = "patient";
    request["age"] = age;
    request["phone"] = phone;
    request["address"] = address;
    m_tcpClient->sendRequest(request);
}

void LoginWidget::onResponseReceived(const QJsonObject &response)
{
    QString type = response["type"].toString();
    bool success = response["success"].toBool();

    if (type == "login_response")
    {
        if (success)
        {
            QString role = response["role"].toString();
            QString username = response["username"].toString();
            if (role == "doctor")
            {
                emit doctorLoggedIn(username);
            }
            else
            {
                emit patientLoggedIn(username);
            }
        }
        else
        {
            QMessageBox::warning(this, "登录失败", "用户名或密码错误！");
        }
    }
    else if (type == "register_response")
    {
        if (success)
        {
            QMessageBox::information(this, "注册成功", "注册成功！请返回登录。");
            if (response["role"].toString() == "doctor")
            {
                showDoctorLogin();
            }
            else
            {
                showPatientLogin();
            }
        }
        else
        {
            QMessageBox::warning(this, "注册失败", "注册失败！可能是用户名已存在。");
        }
    }
}