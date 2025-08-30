#include "loginwidget.h"
#include "doctorinfowidget.h"
#include "patientinfowidget.h"
#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"
#include <QFont>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QToolButton>
#include <QButtonGroup>

LoginWidget::LoginWidget(QWidget* parent)
    : QWidget(parent)
{
    // 仅登录界面应用本地样式
    setObjectName("LoginRoot");
    QFile qss(":/login.qss");
    if (qss.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
    m_communicationClient = new CommunicationClient(this);
    connect(m_communicationClient, &CommunicationClient::connected, this, [this](){
        QMessageBox::information(this, "连接成功", "已成功连接到服务器。");
    });
    connect(m_communicationClient, &CommunicationClient::disconnected, this, [this](){
        QMessageBox::warning(this, "连接断开", "与服务器的连接已断开。");
    });
    m_communicationClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
    connect(m_communicationClient, &CommunicationClient::jsonReceived, this, &LoginWidget::onResponseReceived);


    // Main layout containing only the stacked widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(stackedWidget);

    // --- Page 0: Initial Role Selection Page ---
    QWidget* selectionPage = new QWidget(this);
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionPage);

    QLabel* userTypeLabel = new QLabel("请选择您的身份：", this);
    userTypeLabel->setObjectName("UserTypeLabel");
    QFont userTypeFont;
    userTypeFont.setPointSize(20);
    userTypeLabel->setFont(userTypeFont);
    userTypeLabel->setAlignment(Qt::AlignCenter);

    // 身份卡片：上圆形图标 + 下按钮
    auto makeRoleCard = [&](const QString &icon, const QString &btnText, const QString &btnObjName, std::function<void()> onClick){
        QWidget *card = new QWidget(this);
        card->setObjectName("RoleCard");
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(12,12,12,12);
        cl->setSpacing(12);
        // 圆形图标容器
        QLabel *circle = new QLabel(card);
        circle->setObjectName("RoleCircle");
        circle->setFixedSize(120,120);
        circle->setAlignment(Qt::AlignCenter);
        QPixmap pm = QIcon(icon).pixmap(72,72);
        circle->setPixmap(pm);
        cl->addWidget(circle, 0, Qt::AlignHCenter);
        // 下方动作按钮
        QPushButton *btn = new QPushButton(btnText, card);
        btn->setObjectName(btnObjName);
        btn->setFixedSize(160, 40);
        cl->addWidget(btn, 0, Qt::AlignHCenter);
        QObject::connect(btn, &QPushButton::clicked, card, [onClick]{ onClick(); });
        return card;
    };

    QWidget *doctorCard = makeRoleCard(":/icons/医生.svg", QStringLiteral("我是医生"), "RoleBtnDoctor", [this]{ showDoctorLogin(); });
    QWidget *patientCard = makeRoleCard(":/icons/病人.svg", QStringLiteral("我是病人"), "RoleBtnPatient", [this]{ showPatientLogin(); });

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(doctorCard);
    btnLayout->addSpacing(40);
    btnLayout->addWidget(patientCard);
    btnLayout->addStretch();

    // 外层带圆角边框的容器，包住提示文字与身份选择
    QWidget* selectionGroup = new QWidget(selectionPage);
    selectionGroup->setObjectName("SelectionGroup");
    QVBoxLayout* groupLayout = new QVBoxLayout(selectionGroup);
    groupLayout->setContentsMargins(24, 24, 24, 24);
    groupLayout->setSpacing(16);
    groupLayout->addWidget(userTypeLabel);
    groupLayout->addSpacing(12);
    groupLayout->addLayout(btnLayout);

    selectionLayout->addStretch();
    selectionLayout->addWidget(createTitleLabel()); // Use helper for title
    selectionLayout->addSpacing(20);
    selectionLayout->addWidget(selectionGroup, 0, Qt::AlignHCenter);
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

    // 左图右表单卡片
    QHBoxLayout* doctorContent = new QHBoxLayout();
    doctorContent->setSpacing(32);
    // 左侧插图区域
    QLabel* doctorIllustration = new QLabel(doctorLoginPage);
    doctorIllustration->setObjectName("LoginIllustration");
    doctorIllustration->setMinimumSize(360, 300);
    doctorIllustration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    doctorIllustration->setAlignment(Qt::AlignCenter);
    doctorContent->addWidget(doctorIllustration, 1);
    // 右侧表单卡片
    QWidget* doctorLoginCard = new QWidget(doctorLoginPage);
    doctorLoginCard->setObjectName("LoginFormCard");
    doctorLoginCard->setFixedWidth(380);
    QVBoxLayout* doctorCardLayout = new QVBoxLayout(doctorLoginCard);
    doctorCardLayout->setSpacing(14);
    doctorCardLayout->setContentsMargins(24, 24, 24, 24);
    doctorCardLayout->addWidget(createTitleLabel());
    doctorCardLayout->addSpacing(6);
    doctorCardLayout->addLayout(doctorFormLayout);
    doctorCardLayout->addSpacing(6);
    doctorCardLayout->addWidget(doctorLoginButton);
    doctorCardLayout->addWidget(goToDoctorRegisterButton);
    doctorCardLayout->addWidget(backFromDoctorLoginButton);
    doctorContent->addWidget(doctorLoginCard, 0);
    doctorLoginLayout->addStretch();
    doctorLoginLayout->addLayout(doctorContent);
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

    // 左图右表单卡片
    QHBoxLayout* patientContent = new QHBoxLayout();
    patientContent->setSpacing(32);
    QLabel* patientIllustration = new QLabel(patientLoginPage);
    patientIllustration->setObjectName("LoginIllustration");
    patientIllustration->setMinimumSize(360, 300);
    patientIllustration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    patientIllustration->setAlignment(Qt::AlignCenter);
    patientContent->addWidget(patientIllustration, 1);
    QWidget* patientLoginCard = new QWidget(patientLoginPage);
    patientLoginCard->setObjectName("LoginFormCard");
    patientLoginCard->setFixedWidth(380);
    QVBoxLayout* patientCardLayout = new QVBoxLayout(patientLoginCard);
    patientCardLayout->setSpacing(14);
    patientCardLayout->setContentsMargins(24, 24, 24, 24);
    patientCardLayout->addWidget(createTitleLabel());
    patientCardLayout->addSpacing(6);
    patientCardLayout->addLayout(patientFormLayout);
    patientCardLayout->addSpacing(6);
    patientCardLayout->addWidget(patientLoginButton);
    patientCardLayout->addWidget(goToPatientRegisterButton);
    patientCardLayout->addWidget(backFromPatientLoginButton);
    patientContent->addWidget(patientLoginCard, 0);
    patientLoginLayout->addStretch();
    patientLoginLayout->addLayout(patientContent);
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
    // 进入对应登录页的连接在卡片中已绑定
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
    titleLabel->setObjectName("LoginTitle");
    // 最小兜底：若 QSS 未加载，也有较大字号
    QFont f; f.setPointSize(34); f.setBold(true); titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignCenter);
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

void LoginWidget::onDoctorLogin()
{
    QString username = doctorLoginNameEdit->text();
    QString password = doctorLoginPasswordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空。");
        return;
    }

    QJsonObject request;
    request["action"] = "login";
    request["username"] = username;
    request["password"] = password;
    m_communicationClient->sendJson(request);
}

void LoginWidget::onPatientLogin()
{
    QString username = patientLoginNameEdit->text();
    QString password = patientLoginPasswordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空。");
        return;
    }

    QJsonObject request;
    request["action"] = "login";
    request["username"] = username;
    request["password"] = password;
    m_communicationClient->sendJson(request);
}

void LoginWidget::onDoctorRegister()
{
    QString username = doctorRegisterNameEdit->text();
    QString password = doctorRegisterPasswordEdit->text();
    QString department = doctorDepartmentEdit->text();
    QString phone = doctorPhoneEdit->text();

    if (username.isEmpty() || password.isEmpty() || department.isEmpty() || phone.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "所有字段都不能为空。");
        return;
    }

    QJsonObject request;
    request["action"] = "register";
    request["username"] = username;
    request["password"] = password;
    request["role"] = "doctor";
    // 添加缺失的详细信息
    request["department"] = department;
    request["phone"] = phone;
    m_communicationClient->sendJson(request);
}

void LoginWidget::onPatientRegister()
{
    QString username = patientRegisterNameEdit->text();
    QString password = patientRegisterPasswordEdit->text();
    QString ageStr = patientAgeEdit->text();
    QString phone = patientPhoneEdit->text();
    QString address = patientAddressEdit->text();

    if (username.isEmpty() || password.isEmpty() || ageStr.isEmpty() || phone.isEmpty() || address.isEmpty()) {
        QMessageBox::warning(this, "注册失败", "所有字段都不能为空。");
        return;
    }

    // 验证年龄是否为有效数字
    bool isAgeValid;
    int age = ageStr.toInt(&isAgeValid);
    if (!isAgeValid || age <= 0) {
        QMessageBox::warning(this, "注册失败", "请输入有效的年龄。");
        return;
    }

    QJsonObject request;
    request["action"] = "register";
    request["username"] = username;
    request["password"] = password;
    request["role"] = "patient";
    // 添加缺失的详细信息
    request["age"] = age;
    request["phone"] = phone;
    request["address"] = address;
    m_communicationClient->sendJson(request);
}

// ...existing code...
void LoginWidget::onResponseReceived(const QJsonObject &response)
{
    QString type = response["type"].toString();
    bool success = response["success"].toBool();
    QString message = response["message"].toString(); // 获取服务器返回的消息

    if (type == "login_response") {
        if (success) {
            QMessageBox::information(this, "登录成功", "登录成功！");
            const QString role = response["role"].toString();
            if (role == "doctor") {
                emit doctorLoggedIn(doctorLoginNameEdit->text());
            } else if (role == "patient") {
                emit patientLoggedIn(patientLoginNameEdit->text());
            }
        } else {
            // 使用服务器返回的登录失败消息
            QMessageBox::warning(this, "登录失败", message.isEmpty() ? "用户名或密码错误。" : message);
        }
    } else if (type == "register_response") {
        if (success) {
            QMessageBox::information(this, "注册成功", message.isEmpty() ? "注册成功！请返回登录。" : message);
        } else {
            // 使用服务器返回的注册失败消息
            QMessageBox::warning(this, "注册失败", message.isEmpty() ? "注册失败，请重试。" : message);
        }
    }
}