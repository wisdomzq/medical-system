#include "profilewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTimeEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QFile>
#include <QTime>
#include <QJsonObject>
#include <QJsonArray>
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/doctorprofileservice.h"

ProfileWidget::ProfileWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName), client_(client)
{
    // 加载专用样式
    if (QFile::exists(":/doctor_profile.qss")) {
        QFile f(":/doctor_profile.qss");
        if (f.open(QIODevice::ReadOnly)) this->setStyleSheet(QString::fromUtf8(f.readAll()));
    }

    // 根布局
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);

    // 顶栏
    auto* topBar = new QWidget(this);
    topBar->setObjectName("profileTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    auto* topBarLy = new QHBoxLayout(topBar);
    topBarLy->setContentsMargins(16, 12, 16, 12);
    topBarLy->setSpacing(10);
    auto* title = new QLabel("医生个人信息", topBar);
    title->setObjectName("profileTitle");
    auto* subtitle = new QLabel(doctorName_, topBar);
    subtitle->setObjectName("profileSubtitle");
    auto* titleBox = new QVBoxLayout();
    titleBox->setContentsMargins(0,0,0,0);
    titleBox->setSpacing(2);
    titleBox->addWidget(title);
    titleBox->addWidget(subtitle);
    topBarLy->addLayout(titleBox);
    topBarLy->addStretch();
    root->addWidget(topBar);

    // 内容卡片
    auto* card = new QWidget(this);
    card->setObjectName("profileCard");
    card->setAttribute(Qt::WA_StyledBackground, true);
    auto* cardLy = new QVBoxLayout(card);
    cardLy->setContentsMargins(16, 16, 16, 16);
    cardLy->setSpacing(12);

    // 表单网格（左：标签+字段，右：头像列）
    auto* formGrid = new QGridLayout();
    formGrid->setContentsMargins(0,0,0,0);
    formGrid->setHorizontalSpacing(16);
    formGrid->setVerticalSpacing(10);

    // 左侧字段
    workNumberEdit_ = new QLineEdit(this);
    departmentEdit_ = new QLineEdit(this);
    bioEdit_ = new QTextEdit(this);
    bioEdit_->setPlaceholderText("个人资料……");

    workStartEdit_ = new QTimeEdit(this); workStartEdit_->setDisplayFormat("HH:mm");
    workEndEdit_   = new QTimeEdit(this); workEndEdit_->setDisplayFormat("HH:mm");
    feeEdit_ = new QDoubleSpinBox(this); feeEdit_->setDecimals(2); feeEdit_->setRange(0, 100000); feeEdit_->setSuffix(" 元");
    dailyLimitEdit_ = new QSpinBox(this); dailyLimitEdit_->setRange(0, 10000); dailyLimitEdit_->setSuffix(" 人/日");

    int r = 0;
    formGrid->addWidget(new QLabel("工号："), r, 0, Qt::AlignRight); formGrid->addWidget(workNumberEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("科室："), r, 0, Qt::AlignRight); formGrid->addWidget(departmentEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("个人资料："), r, 0, Qt::AlignRight | Qt::AlignTop); formGrid->addWidget(bioEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("上班开始："), r, 0, Qt::AlignRight); formGrid->addWidget(workStartEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("上班结束："), r, 0, Qt::AlignRight); formGrid->addWidget(workEndEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("挂号费用："), r, 0, Qt::AlignRight); formGrid->addWidget(feeEdit_, r, 1); r++;
    formGrid->addWidget(new QLabel("单日上限："), r, 0, Qt::AlignRight); formGrid->addWidget(dailyLimitEdit_, r, 1); r++;

    // 右侧头像列
    photoPreview_ = new QLabel(this);
    photoPreview_->setObjectName("photoPreview");
    photoPreview_->setFixedSize(120,120);
    photoPreview_->setAlignment(Qt::AlignCenter);
    photoPreview_->setAttribute(Qt::WA_StyledBackground, true);
    // 外层边框容器，保证边框可见
    auto* photoFrame = new QWidget(this);
    photoFrame->setObjectName("photoFrame");
    photoFrame->setAttribute(Qt::WA_StyledBackground, true);
    photoFrame->setFixedSize(128,128);
    auto* photoFrameLy = new QVBoxLayout(photoFrame);
    photoFrameLy->setContentsMargins(4,4,4,4); // 内边距让边框更明显
    photoFrameLy->addWidget(photoPreview_, 0, Qt::AlignCenter);
    auto* choosePhotoBtn = new QPushButton("选择照片", this);
    choosePhotoBtn->setObjectName("choosePhotoBtn");
    auto* photoColW = new QWidget(this);
    photoColW->setObjectName("photoCol");
    auto* photoCol = new QVBoxLayout(photoColW);
    photoCol->setContentsMargins(0,0,0,0);
    photoCol->setSpacing(8);
    photoCol->addWidget(photoFrame, 0, Qt::AlignLeft);
    photoCol->addWidget(choosePhotoBtn, 0, Qt::AlignLeft);
    photoCol->addStretch();
    formGrid->addWidget(photoColW, 0, 2, r, 1);
    formGrid->setColumnStretch(1, 1);
    formGrid->setColumnStretch(2, 0);

    cardLy->addLayout(formGrid);

    // 操作区
    auto* actions = new QWidget(this);
    actions->setObjectName("profileActions");
    auto* btns = new QHBoxLayout(actions);
    btns->setContentsMargins(0, 8, 0, 0);
    btns->addStretch();
    refreshBtn_ = new QPushButton("刷新", this); refreshBtn_->setObjectName("refreshBtn");
    saveBtn_ = new QPushButton("保存", this); saveBtn_->setObjectName("saveBtn");
    btns->addWidget(refreshBtn_);
    btns->addWidget(saveBtn_);
    cardLy->addWidget(actions);

    root->addWidget(card);
    root->addStretch();

    connect(choosePhotoBtn, &QPushButton::clicked, this, &ProfileWidget::onChoosePhoto);
    connect(saveBtn_, &QPushButton::clicked, this, &ProfileWidget::onSave);
    connect(refreshBtn_, &QPushButton::clicked, this, &ProfileWidget::onRefresh);
    // 网络连上时自动拉取；进入页面立即拉取一次
    if (client_) connect(client_, &CommunicationClient::connected, this, &ProfileWidget::onConnected);

    service_ = new DoctorProfileService(client_, this);
    connect(service_, &DoctorProfileService::infoReceived, this, [this](const QJsonObject& d){
        workNumberEdit_->setText(d.value("work_number").toString());
        departmentEdit_->setText(d.value("department").toString());
        bioEdit_->setPlainText(d.value("specialization").toString());
        feeEdit_->setValue(d.value("consultation_fee").toDouble());
        dailyLimitEdit_->setValue(d.value("max_patients_per_day").toInt());
        parseWorkTitle(d.value("title").toString());
        const auto photoB64 = d.value("photo").toString();
        if (!photoB64.isEmpty()) {
            photoBytes_ = QByteArray::fromBase64(photoB64.toUtf8());
            QPixmap px; px.loadFromData(photoBytes_);
            photoPreview_->setPixmap(px.scaled(photoPreview_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });
    connect(service_, &DoctorProfileService::infoFailed, this, [this](const QString& msg){
        if (!msg.isEmpty()) QMessageBox::warning(this, "失败", msg);
    });
    connect(service_, &DoctorProfileService::updateResult, this, [this](bool ok, const QString& msg){
        if (ok) QMessageBox::information(this, "提示", msg.isEmpty() ? QStringLiteral("保存成功") : msg);
        else QMessageBox::warning(this, "失败", msg.isEmpty() ? QStringLiteral("保存失败") : msg);
    });

    // 进入页面立即请求一次资料
    requestProfile();
}

void ProfileWidget::onConnected() { requestProfile(); }

void ProfileWidget::requestProfile() { service_->requestDoctorInfo(doctorName_); }

void ProfileWidget::onRefresh() { requestProfile(); }

// 所有数据更新由 DoctorProfileService 信号驱动，无需直接处理 JSON

QString ProfileWidget::buildWorkTitle() const {
    // 将时间以 "HH:mm-HH:mm" 编码到 title 字段（数据库现有列）
    const QString s = workStartEdit_->time().toString("HH:mm");
    const QString e = workEndEdit_->time().toString("HH:mm");
    return s + "-" + e;
}

void ProfileWidget::parseWorkTitle(const QString& title) {
    const auto parts = title.split('-');
    if (parts.size() == 2) {
        workStartEdit_->setTime(QTime::fromString(parts[0], "HH:mm"));
        workEndEdit_->setTime(QTime::fromString(parts[1], "HH:mm"));
    }
}

void ProfileWidget::onChoosePhoto() {
    const auto path = QFileDialog::getOpenFileName(this, "选择照片", QString(), "Images (*.png *.jpg *.jpeg)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        photoBytes_ = f.readAll();
        QPixmap px; px.loadFromData(photoBytes_);
        photoPreview_->setPixmap(px.scaled(photoPreview_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

void ProfileWidget::onSave() {
    // 将 UI 数据映射回 DB 字段
    QJsonObject data;
    data["name"] = doctorName_; // 保持用户名即姓名模型
    data["department"] = departmentEdit_->text();
    data["phone"] = QJsonValue::Null; // 此页未编辑电话
    data["email"] = QJsonValue::Null;
    data["work_number"] = workNumberEdit_->text();
    data["title"] = buildWorkTitle(); // 用 title 存上下班时段
    data["specialization"] = bioEdit_->toPlainText(); // 个人资料
    data["consultation_fee"] = feeEdit_->value();
    data["max_patients_per_day"] = dailyLimitEdit_->value();
    // 照片：若已选择，随请求以 base64 发送（后端 BLOB 列 photo）
    if (!photoBytes_.isEmpty()) {
        data["photo"] = QString::fromUtf8(photoBytes_.toBase64());
    }

    service_->updateDoctorInfo(doctorName_, data);
}
