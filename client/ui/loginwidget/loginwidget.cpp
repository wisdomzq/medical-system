#include "loginwidget.h"
#include "doctorinfowidget.h"
#include "patientinfowidget.h"
#include "core/services/authservice.h"
#include "core/network/protocol.h"
#include <QFont>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QToolButton>
#include <QButtonGroup>

LoginWidget::~LoginWidget() {}

LoginWidget::LoginWidget(CommunicationClient* sharedClient, QWidget* parent)
    : QWidget(parent)
{
    // 仅登录界面应用本地样式
    setObjectName("LoginRoot");
    QFile qss(":/login.qss");
    if (qss.open(QIODevice::ReadOnly)) {
        setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
    // 使用共享认证服务客户端
    m_authService = new AuthService(sharedClient, this);
    connect(m_authService, &AuthService::connected, this, [this]() {
        QMessageBox::information(this, "连接成功", "已成功连接到服务器。");
    });
    connect(m_authService, &AuthService::disconnected, this, [this]() {
        QMessageBox::warning(this, "连接断开", "与服务器的连接已断开。");
    });
    connect(m_authService, &AuthService::networkError, this, [this](int, const QString& msg) {
        QMessageBox::critical(this, "网络错误", msg);
    });
    // 登录、注册结果与 UI 交互
    connect(m_authService, &AuthService::loginSucceeded, this, [this](const QString& role, const QString& username, const QString& message) {
        if (!message.isEmpty())
            QMessageBox::information(this, "登录成功", message);
        if (role == "doctor") emit doctorLoggedIn(username);
        else if (role == "patient") emit patientLoggedIn(username);
        else QMessageBox::information(this, "登录", QStringLiteral("登录成功"));
    });
    connect(m_authService, &AuthService::loginFailed, this, [this](const QString& message) {
        QMessageBox::warning(this, "登录失败", message);
    });
    connect(m_authService, &AuthService::registerSucceeded, this, [this](const QString& /*role*/, const QString& message) {
        QMessageBox::information(this, "注册成功", message.isEmpty() ? QStringLiteral("注册成功！请返回登录。") : message);
    });
    connect(m_authService, &AuthService::registerFailed, this, [this](const QString& message) {
        QMessageBox::warning(this, "注册失败", message.isEmpty() ? QStringLiteral("注册失败，请重试。") : message);
    });
    // 共享客户端由外部统一连接，这里不再 connectDefault

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
        card->setMinimumSize(300, 380);
        card->setMaximumSize(320, 400);
        
        QVBoxLayout *cl = new QVBoxLayout(card);
        cl->setContentsMargins(24,32,24,32);
        cl->setSpacing(16);
        
        // 圆形图标容器
        QLabel *circle = new QLabel(card);
        circle->setObjectName("RoleCircle");
        circle->setFixedSize(140,140);
        circle->setAlignment(Qt::AlignCenter);
        QPixmap pm = QIcon(icon).pixmap(88,88);
        circle->setPixmap(pm);
        cl->addWidget(circle, 0, Qt::AlignHCenter);
        
        // 角色标题
        QLabel *roleName = new QLabel(card);
        roleName->setObjectName("RoleName");
        roleName->setAlignment(Qt::AlignHCenter);
        roleName->setText(btnText == QStringLiteral("我是医生") ? QStringLiteral("医生工作台") : QStringLiteral("患者服务平台"));
        cl->addWidget(roleName);
        
        // 角色描述
        QLabel *roleDesc = new QLabel(card);
        roleDesc->setObjectName("RoleDesc");
        roleDesc->setAlignment(Qt::AlignHCenter);
        roleDesc->setWordWrap(true);
        roleDesc->setText(btnText == QStringLiteral("我是医生")
                          ? QStringLiteral("专业的医疗管理平台\n门诊管理 · 病患跟踪 · 数据分析")
                          : QStringLiteral("便捷的就医服务系统\n在线挂号 · 健康档案 · 医患沟通"));
        cl->addWidget(roleDesc);
        
        cl->addSpacing(4);
        
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
    btnLayout->setContentsMargins(40,0,40,0);
    btnLayout->setSpacing(48);
    btnLayout->addStretch();
    doctorCard->setMinimumWidth(300);
    patientCard->setMinimumWidth(300);
    btnLayout->addWidget(doctorCard);
    btnLayout->addWidget(patientCard);
    btnLayout->addStretch();

    // 外层带圆角边框的容器，包住提示文字与身份选择
    QWidget* selectionGroup = new QWidget(selectionPage);
    selectionGroup->setObjectName("SelectionGroup");
    selectionGroup->setMaximumWidth(900);
    QVBoxLayout* groupLayout = new QVBoxLayout(selectionGroup);
    groupLayout->setContentsMargins(40, 40, 40, 40);
    groupLayout->setSpacing(24);
    groupLayout->addWidget(userTypeLabel);
    // 次级说明文字
    QLabel* userTypeSub = new QLabel(QStringLiteral("选择您的身份类型，开始使用智慧医疗服务"), selectionGroup);
    userTypeSub->setObjectName("UserTypeSubLabel");
    userTypeSub->setAlignment(Qt::AlignCenter);
    groupLayout->addWidget(userTypeSub);
    groupLayout->addSpacing(10);
    groupLayout->addLayout(btnLayout);

    selectionLayout->addStretch();
    selectionLayout->addWidget(createTitleLabel()); // Use helper for title
    selectionLayout->addSpacing(10);
    selectionLayout->addWidget(selectionGroup, 0, Qt::AlignHCenter);
    selectionLayout->addStretch();

    // 以下布局与控件构建保持与默认构造一致
    QWidget* doctorLoginPage = new QWidget(this);
    QVBoxLayout* doctorLoginLayout = new QVBoxLayout(doctorLoginPage);
    doctorLoginNameEdit = new QLineEdit(this);
    doctorLoginPasswordEdit = new QLineEdit(this);
    doctorLoginNameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    doctorLoginPasswordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    doctorLoginNameEdit->setClearButtonEnabled(true);
    doctorLoginPasswordEdit->setClearButtonEnabled(true);
    doctorLoginPasswordEdit->setEchoMode(QLineEdit::Password);
    doctorLoginButton = new QPushButton(QStringLiteral("立即登录"), this);
    doctorLoginButton->setObjectName("PrimaryLoginBtn");
    goToDoctorRegisterButton = new QPushButton(QStringLiteral("没有账号？ 立即注册"), this);
    goToDoctorRegisterButton->setObjectName("RegisterLink");
    goToDoctorRegisterButton->setFlat(true);
    backFromDoctorLoginButton = new QPushButton("返回", this);

    auto makeUnderRow = [](QLineEdit* edit, const QString& iconRes, QWidget* parent){
        QWidget* row = new QWidget(parent);
        row->setObjectName("UnderRow");
        QHBoxLayout* hl = new QHBoxLayout(row);
        hl->setContentsMargins(0,0,0,0);
        hl->setSpacing(8);
        QLabel* icon = new QLabel(row);
        icon->setObjectName("InputIcon");
        icon->setFixedSize(24,24);
        icon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        icon->setPixmap(QIcon(iconRes).pixmap(18,18));
        edit->setFrame(false);
        hl->addWidget(icon);
        hl->addWidget(edit, 1);
        return row;
    };

    QHBoxLayout* doctorContent = new QHBoxLayout();
    doctorContent->setSpacing(32);
    QLabel* doctorIllustration = new QLabel(doctorLoginPage);
    doctorIllustration->setObjectName("LoginIllustration");
    doctorIllustration->setMinimumSize(360, 300);
    doctorIllustration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    doctorIllustration->setAlignment(Qt::AlignCenter);
    doctorContent->addWidget(doctorIllustration, 1);
    QWidget* doctorLoginCard = new QWidget(doctorLoginPage);
    doctorLoginCard->setObjectName("LoginFormCard");
    doctorLoginCard->setFixedWidth(380);
    QVBoxLayout* doctorCardLayout = new QVBoxLayout(doctorLoginCard);
    doctorCardLayout->setSpacing(14);
    doctorCardLayout->setContentsMargins(24, 24, 24, 24);
    doctorCardLayout->addWidget(createTitleLabel());
    doctorCardLayout->addSpacing(6);
    doctorCardLayout->addWidget(makeUnderRow(doctorLoginNameEdit, ":/icons/用户名.svg", doctorLoginCard));
    doctorCardLayout->addWidget(makeUnderRow(doctorLoginPasswordEdit, ":/icons/密码.svg", doctorLoginCard));
    doctorCardLayout->addSpacing(6);
    doctorCardLayout->addWidget(doctorLoginButton);
    QHBoxLayout* linksDoc = new QHBoxLayout();
    backFromDoctorLoginButton->setText(QString());
    backFromDoctorLoginButton->setObjectName("BackIconBtn");
    backFromDoctorLoginButton->setIcon(QIcon(":/icons/返回.svg"));
    backFromDoctorLoginButton->setIconSize(QSize(18,18));
    backFromDoctorLoginButton->setFlat(true);
    linksDoc->addWidget(backFromDoctorLoginButton, 0, Qt::AlignLeft);
    linksDoc->addStretch();
    linksDoc->addWidget(goToDoctorRegisterButton, 0, Qt::AlignRight);
    doctorCardLayout->addLayout(linksDoc);
    doctorContent->addWidget(doctorLoginCard, 0);
    doctorLoginLayout->addStretch();
    doctorLoginLayout->addLayout(doctorContent);
    doctorLoginLayout->addStretch();

    QWidget* patientLoginPage = new QWidget(this);
    QVBoxLayout* patientLoginLayout = new QVBoxLayout(patientLoginPage);
    patientLoginNameEdit = new QLineEdit(this);
    patientLoginPasswordEdit = new QLineEdit(this);
    patientLoginNameEdit->setPlaceholderText(QStringLiteral("请输入用户名"));
    patientLoginPasswordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    patientLoginNameEdit->setClearButtonEnabled(true);
    patientLoginPasswordEdit->setClearButtonEnabled(true);
    patientLoginPasswordEdit->setEchoMode(QLineEdit::Password);
    patientLoginButton = new QPushButton(QStringLiteral("立即登录"), this);
    patientLoginButton->setObjectName("PrimaryLoginBtn");
    goToPatientRegisterButton = new QPushButton(QStringLiteral("没有账号？ 立即注册"), this);
    goToPatientRegisterButton->setObjectName("RegisterLink");
    goToPatientRegisterButton->setFlat(true);
    backFromPatientLoginButton = new QPushButton("返回", this);

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
    patientCardLayout->addWidget(makeUnderRow(patientLoginNameEdit, ":/icons/用户名.svg", patientLoginCard));
    patientCardLayout->addWidget(makeUnderRow(patientLoginPasswordEdit, ":/icons/密码.svg", patientLoginCard));
    patientCardLayout->addSpacing(6);
    patientCardLayout->addWidget(patientLoginButton);
    QHBoxLayout* linksPat = new QHBoxLayout();
    backFromPatientLoginButton->setText(QString());
    backFromPatientLoginButton->setObjectName("BackIconBtn");
    backFromPatientLoginButton->setIcon(QIcon(":/icons/返回.svg"));
    backFromPatientLoginButton->setIconSize(QSize(18,18));
    backFromPatientLoginButton->setFlat(true);
    linksPat->addWidget(backFromPatientLoginButton, 0, Qt::AlignLeft);
    linksPat->addStretch();
    linksPat->addWidget(goToPatientRegisterButton, 0, Qt::AlignRight);
    patientCardLayout->addLayout(linksPat);
    patientContent->addWidget(patientLoginCard, 0);
    patientLoginLayout->addStretch();
    patientLoginLayout->addLayout(patientContent);
    patientLoginLayout->addStretch();

    QWidget* doctorRegisterPage = new QWidget(this);
    QVBoxLayout* doctorRegisterLayout = new QVBoxLayout(doctorRegisterPage);
    doctorRegisterNameEdit = new QLineEdit(this);
    doctorRegisterPasswordEdit = new QLineEdit(this);
    doctorRegisterPasswordEdit->setEchoMode(QLineEdit::Password);
    doctorDepartmentEdit = new QLineEdit(this);
    doctorPhoneEdit = new QLineEdit(this);
    doctorRegisterNameEdit->setPlaceholderText(QStringLiteral("请输入姓名/用户名"));
    doctorRegisterPasswordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    doctorDepartmentEdit->setPlaceholderText(QStringLiteral("请输入科室"));
    doctorPhoneEdit->setPlaceholderText(QStringLiteral("请输入电话"));
    doctorRegisterNameEdit->setClearButtonEnabled(true);
    doctorRegisterPasswordEdit->setClearButtonEnabled(true);
    doctorDepartmentEdit->setClearButtonEnabled(true);
    doctorPhoneEdit->setClearButtonEnabled(true);
    doctorRegisterButton = new QPushButton("注册", this);
    doctorRegisterButton->setObjectName("PrimaryLoginBtn");
    backToDoctorLoginButton = new QPushButton(QStringLiteral("返回登录"), this);
    backToDoctorLoginButton->setObjectName("RegisterLink");
    backToDoctorLoginButton->setFlat(true);

    QHBoxLayout* doctorRegContent = new QHBoxLayout();
    doctorRegContent->setSpacing(32);
    QLabel* doctorRegIllustration = new QLabel(doctorRegisterPage);
    doctorRegIllustration->setObjectName("LoginIllustration");
    doctorRegIllustration->setMinimumSize(360, 300);
    doctorRegIllustration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    doctorRegIllustration->setAlignment(Qt::AlignCenter);
    doctorRegContent->addWidget(doctorRegIllustration, 1);

    QWidget* doctorRegisterCard = new QWidget(doctorRegisterPage);
    doctorRegisterCard->setObjectName("LoginFormCard");
    doctorRegisterCard->setFixedWidth(380);
    QVBoxLayout* doctorRegisterCardLayout = new QVBoxLayout(doctorRegisterCard);
    doctorRegisterCardLayout->setSpacing(14);
    doctorRegisterCardLayout->setContentsMargins(24,24,24,24);
    doctorRegisterCardLayout->addWidget(createTitleLabel());
    doctorRegisterCardLayout->addSpacing(6);
    doctorRegisterCardLayout->addWidget(makeUnderRow(doctorRegisterNameEdit, ":/icons/用户名.svg", doctorRegisterCard));
    doctorRegisterCardLayout->addWidget(makeUnderRow(doctorRegisterPasswordEdit, ":/icons/密码.svg", doctorRegisterCard));
    doctorRegisterCardLayout->addWidget(makeUnderRow(doctorDepartmentEdit, ":/icons/科室.svg", doctorRegisterCard));
    doctorRegisterCardLayout->addWidget(makeUnderRow(doctorPhoneEdit, ":/icons/电话.svg", doctorRegisterCard));
    doctorRegisterCardLayout->addSpacing(6);
    doctorRegisterCardLayout->addWidget(doctorRegisterButton);
    QHBoxLayout* linksDocReg = new QHBoxLayout();
    QPushButton* backIconDocReg = new QPushButton(doctorRegisterCard);
    backIconDocReg->setObjectName("BackIconBtn");
    backIconDocReg->setIcon(QIcon(":/icons/返回.svg"));
    backIconDocReg->setIconSize(QSize(18,18));
    backIconDocReg->setFlat(true);
    linksDocReg->addWidget(backIconDocReg, 0, Qt::AlignLeft);
    linksDocReg->addStretch();
    linksDocReg->addWidget(backToDoctorLoginButton, 0, Qt::AlignRight);
    doctorRegisterCardLayout->addLayout(linksDocReg);

    QObject::connect(backIconDocReg, &QPushButton::clicked, this, &LoginWidget::showDoctorLogin);

    doctorRegContent->addWidget(doctorRegisterCard, 0);
    doctorRegisterLayout->addStretch();
    doctorRegisterLayout->addLayout(doctorRegContent);
    doctorRegisterLayout->addStretch();

    QWidget* patientRegisterPage = new QWidget(this);
    QVBoxLayout* patientRegisterLayout = new QVBoxLayout(patientRegisterPage);
    patientRegisterNameEdit = new QLineEdit(this);
    patientRegisterPasswordEdit = new QLineEdit(this);
    patientRegisterPasswordEdit->setEchoMode(QLineEdit::Password);
    patientAgeEdit = new QLineEdit(this);
    patientPhoneEdit = new QLineEdit(this);
    patientAddressEdit = new QLineEdit(this);
    patientRegisterNameEdit->setPlaceholderText(QStringLiteral("请输入姓名/用户名"));
    patientRegisterPasswordEdit->setPlaceholderText(QStringLiteral("请输入密码"));
    patientAgeEdit->setPlaceholderText(QStringLiteral("请输入年龄"));
    patientPhoneEdit->setPlaceholderText(QStringLiteral("请输入电话"));
    patientAddressEdit->setPlaceholderText(QStringLiteral("请输入地址"));
    patientRegisterNameEdit->setClearButtonEnabled(true);
    patientRegisterPasswordEdit->setClearButtonEnabled(true);
    patientAgeEdit->setClearButtonEnabled(true);
    patientPhoneEdit->setClearButtonEnabled(true);
    patientAddressEdit->setClearButtonEnabled(true);
    patientRegisterButton = new QPushButton("注册", this);
    patientRegisterButton->setObjectName("PrimaryLoginBtn");
    backToPatientLoginButton = new QPushButton(QStringLiteral("返回登录"), this);
    backToPatientLoginButton->setObjectName("RegisterLink");
    backToPatientLoginButton->setFlat(true);

    QHBoxLayout* patientRegContent = new QHBoxLayout();
    patientRegContent->setSpacing(32);
    QLabel* patientRegIllustration = new QLabel(patientRegisterPage);
    patientRegIllustration->setObjectName("LoginIllustration");
    patientRegIllustration->setMinimumSize(360, 300);
    patientRegIllustration->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    patientRegIllustration->setAlignment(Qt::AlignCenter);
    patientRegContent->addWidget(patientRegIllustration, 1);

    QWidget* patientRegisterCard = new QWidget(patientRegisterPage);
    patientRegisterCard->setObjectName("LoginFormCard");
    patientRegisterCard->setFixedWidth(380);
    QVBoxLayout* patientRegisterCardLayout = new QVBoxLayout(patientRegisterCard);
    patientRegisterCardLayout->setSpacing(14);
    patientRegisterCardLayout->setContentsMargins(24,24,24,24);
    patientRegisterCardLayout->addWidget(createTitleLabel());
    patientRegisterCardLayout->addSpacing(6);
    patientRegisterCardLayout->addWidget(makeUnderRow(patientRegisterNameEdit, ":/icons/用户名.svg", patientRegisterCard));
    patientRegisterCardLayout->addWidget(makeUnderRow(patientRegisterPasswordEdit, ":/icons/密码.svg", patientRegisterCard));
    patientRegisterCardLayout->addWidget(makeUnderRow(patientAgeEdit, ":/icons/年龄.svg", patientRegisterCard));
    patientRegisterCardLayout->addWidget(makeUnderRow(patientPhoneEdit, ":/icons/电话.svg", patientRegisterCard));
    patientRegisterCardLayout->addWidget(makeUnderRow(patientAddressEdit, ":/icons/地址.svg", patientRegisterCard));
    patientRegisterCardLayout->addSpacing(6);
    patientRegisterCardLayout->addWidget(patientRegisterButton);
    QHBoxLayout* linksPatReg = new QHBoxLayout();
    QPushButton* backIconPatReg = new QPushButton(patientRegisterCard);
    backIconPatReg->setObjectName("BackIconBtn");
    backIconPatReg->setIcon(QIcon(":/icons/返回.svg"));
    backIconPatReg->setIconSize(QSize(18,18));
    backIconPatReg->setFlat(true);
    linksPatReg->addWidget(backIconPatReg, 0, Qt::AlignLeft);
    linksPatReg->addStretch();
    linksPatReg->addWidget(backToPatientLoginButton, 0, Qt::AlignRight);
    patientRegisterCardLayout->addLayout(linksPatReg);

    QObject::connect(backIconPatReg, &QPushButton::clicked, this, &LoginWidget::showPatientLogin);

    patientRegContent->addWidget(patientRegisterCard, 0);
    patientRegisterLayout->addStretch();
    patientRegisterLayout->addLayout(patientRegContent);
    patientRegisterLayout->addStretch();

    // --- Add all pages to the stacked widget ---
    stackedWidget->addWidget(selectionPage);          // Index 0
    stackedWidget->addWidget(doctorLoginPage);        // Index 1
    stackedWidget->addWidget(patientLoginPage);       // Index 2
    stackedWidget->addWidget(doctorRegisterPage);     // Index 3
    stackedWidget->addWidget(patientRegisterPage);    // Index 4

    // --- Connect signals and slots ---
    connect(goToDoctorRegisterButton, &QPushButton::clicked, this, &LoginWidget::showDoctorRegister);
    connect(goToPatientRegisterButton, &QPushButton::clicked, this, &LoginWidget::showPatientRegister);
    connect(backToDoctorLoginButton, &QPushButton::clicked, this, &LoginWidget::showDoctorLogin);
    connect(backToPatientLoginButton, &QPushButton::clicked, this, &LoginWidget::showPatientLogin);
    connect(backFromDoctorLoginButton, &QPushButton::clicked, this, &LoginWidget::showInitialSelection);
    connect(backFromPatientLoginButton, &QPushButton::clicked, this, &LoginWidget::showInitialSelection);

    connect(doctorLoginButton, &QPushButton::clicked, this, &LoginWidget::onDoctorLogin);
    connect(patientLoginButton, &QPushButton::clicked, this, &LoginWidget::onPatientLogin);
    connect(doctorRegisterButton, &QPushButton::clicked, this, &LoginWidget::onDoctorRegister);
    connect(patientRegisterButton, &QPushButton::clicked, this, &LoginWidget::onPatientRegister);
}

QLabel* LoginWidget::createTitleLabel() {
    QLabel* titleLabel = new QLabel("智慧医疗系统");
    titleLabel->setObjectName("LoginTitle");
    
    // 最小兜底：若 QSS 未加载，也有较大字号与指定字体
    QFont f;
    f.setFamily(QStringLiteral("文泉驿微米黑"));
    f.setPointSize(28);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);
    titleLabel->setFont(f);
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

    m_authService->login(username, password);
}

void LoginWidget::onPatientLogin()
{
    QString username = patientLoginNameEdit->text();
    QString password = patientLoginPasswordEdit->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录失败", "用户名和密码不能为空。");
        return;
    }

    m_authService->login(username, password);
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

    m_authService->registerDoctor(username, password, department, phone);
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

    m_authService->registerPatient(username, password, age, phone, address);
}

// 网络响应已由 AuthService 处理