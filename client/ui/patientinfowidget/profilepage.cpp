#include "profilepage.h"
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/patientservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QTimer>
#include <QMessageBox>
#include <QJsonObject>

ProfilePage::ProfilePage(CommunicationClient *c,const QString &patient,QWidget *parent)
    : BasePage(c,patient,parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("profileTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("个人信息", topBar);
    title->setObjectName("profileTitle");
    QLabel* subTitle = new QLabel(QString("患者：%1").arg(patient), topBar);
    subTitle->setObjectName("profileSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    topBarLayout->addStretch();
    root->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);

    // 表单
    auto* form = new QFormLayout();
    
    // 基本信息
    nameEdit_ = new QLineEdit(this);
    ageEdit_ = new QSpinBox(this);
    ageEdit_->setRange(0, 150);
    ageEdit_->setSuffix(" 岁");
    
    genderEdit_ = new QComboBox(this);
    genderEdit_->addItems({"", "男", "女"});
    
    phoneEdit_ = new QLineEdit(this);
    emailEdit_ = new QLineEdit(this);
    addressEdit_ = new QLineEdit(this);
    
    // 身份证和紧急联系人信息
    idCardEdit_ = new QLineEdit(this);
    emergencyContactEdit_ = new QLineEdit(this);
    emergencyPhoneEdit_ = new QLineEdit(this);
    
    form->addRow("姓名：", nameEdit_);
    form->addRow("年龄：", ageEdit_);
    form->addRow("性别：", genderEdit_);
    form->addRow("电话：", phoneEdit_);
    form->addRow("邮箱：", emailEdit_);
    form->addRow("地址：", addressEdit_);
    form->addRow("身份证号：", idCardEdit_);
    form->addRow("紧急联系人：", emergencyContactEdit_);
    form->addRow("紧急联系电话：", emergencyPhoneEdit_);

    contentLayout->addLayout(form);

    // 按钮
    auto* btns = new QHBoxLayout();
    btns->addStretch();
    refreshBtn_ = new QPushButton("刷新", this);
    refreshBtn_->setObjectName("primaryBtn");
    updateBtn_ = new QPushButton("保存", this);
    updateBtn_->setObjectName("primaryBtn");
    QPushButton *backBtn = new QPushButton("返回登录", this);
    backBtn->setObjectName("primaryBtn");
    btns->addWidget(refreshBtn_);
    btns->addWidget(updateBtn_);
    btns->addWidget(backBtn);
    contentLayout->addLayout(btns);
    contentLayout->addStretch();
    
    root->addWidget(contentWidget);

    connect(updateBtn_, &QPushButton::clicked, this, &ProfilePage::updateProfile);
    connect(refreshBtn_, &QPushButton::clicked, this, &ProfilePage::requestPatientInfo);
    connect(backBtn, &QPushButton::clicked, this, [this]{ emit backToLogin(); });

    // 注入服务并连接信号
    patientService_ = new PatientService(m_client, this);
    connect(patientService_, &PatientService::patientInfoReceived, this, [this](const QJsonObject& d){
        nameEdit_->setText(d.value("name").toString());
        ageEdit_->setValue(d.value("age").toInt());
        genderEdit_->setCurrentText(d.value("gender").toString());
        phoneEdit_->setText(d.value("phone").toString());
        emailEdit_->setText(d.value("email").toString());
        addressEdit_->setText(d.value("address").toString());
        idCardEdit_->setText(d.value("id_card").toString());
        emergencyContactEdit_->setText(d.value("emergency_contact").toString());
        emergencyPhoneEdit_->setText(d.value("emergency_phone").toString());
    });
    connect(patientService_, &PatientService::updatePatientInfoResult, this, [this](bool ok, const QString& msg){
        if (ok) QMessageBox::information(this, "提示", msg.isEmpty() ? QStringLiteral("保存成功") : msg);
        else QMessageBox::warning(this, "失败", msg.isEmpty() ? QStringLiteral("保存失败") : msg);
    });

    QTimer::singleShot(200, this, &ProfilePage::requestPatientInfo);
}

void ProfilePage::handleResponse(const QJsonObject &obj){
    if(obj.value("type").toString() == "patient_info_response" && obj.value("success").toBool()){
        QJsonObject d = obj.value("data").toObject();
        nameEdit_->setText(d.value("name").toString());
        ageEdit_->setValue(d.value("age").toInt());
        genderEdit_->setCurrentText(d.value("gender").toString());
        phoneEdit_->setText(d.value("phone").toString());
        emailEdit_->setText(d.value("email").toString());
        addressEdit_->setText(d.value("address").toString());
        idCardEdit_->setText(d.value("id_card").toString());
        emergencyContactEdit_->setText(d.value("emergency_contact").toString());
        emergencyPhoneEdit_->setText(d.value("emergency_phone").toString());
    } else if(obj.value("type").toString() == "update_patient_info_response") {
        if (obj.value("success").toBool()) {
            QMessageBox::information(this, "提示", "保存成功");
        } else {
            QMessageBox::warning(this, "失败", "保存失败");
        }
    }
}

void ProfilePage::requestPatientInfo(){ patientService_->requestPatientInfo(m_patientName); }

void ProfilePage::updateProfile(){ 
    // 将 UI 数据映射到数据库字段
    QJsonObject data;
    data["name"] = nameEdit_->text();
    data["age"] = ageEdit_->value();
    data["gender"] = genderEdit_->currentText();
    data["phone"] = phoneEdit_->text();
    data["email"] = emailEdit_->text();
    data["address"] = addressEdit_->text();
    data["id_card"] = idCardEdit_->text();
    data["emergency_contact"] = emergencyContactEdit_->text();
    data["emergency_phone"] = emergencyPhoneEdit_->text();

    patientService_->updatePatientInfo(m_patientName, data);
}
