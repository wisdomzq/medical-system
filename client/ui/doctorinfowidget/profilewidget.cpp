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
    auto* root = new QVBoxLayout(this);
    auto* title = new QLabel(QString("个人信息 - %1").arg(doctorName_));
    title->setStyleSheet("font-size:18px;font-weight:600;margin:4px 0 12px 0;");
    root->addWidget(title);

    auto* form = new QFormLayout();
    workNumberEdit_ = new QLineEdit(this);
    departmentEdit_ = new QLineEdit(this);
    bioEdit_ = new QTextEdit(this);
    bioEdit_->setPlaceholderText("个人资料……");

    photoPreview_ = new QLabel(this);
    photoPreview_->setFixedSize(96,96);
    photoPreview_->setStyleSheet("border:1px solid #ccc;background:#fafafa;");
    auto* choosePhotoBtn = new QPushButton("选择照片", this);

    auto* photoRowW = new QWidget(this);
    auto* photoRow = new QHBoxLayout(photoRowW);
    photoRow->addWidget(photoPreview_);
    photoRow->addWidget(choosePhotoBtn);
    photoRow->addStretch();

    workStartEdit_ = new QTimeEdit(this); workStartEdit_->setDisplayFormat("HH:mm");
    workEndEdit_   = new QTimeEdit(this); workEndEdit_->setDisplayFormat("HH:mm");

    feeEdit_ = new QDoubleSpinBox(this); feeEdit_->setDecimals(2); feeEdit_->setRange(0, 100000); feeEdit_->setSuffix(" 元");
    dailyLimitEdit_ = new QSpinBox(this); dailyLimitEdit_->setRange(0, 10000); dailyLimitEdit_->setSuffix(" 人/日");

    form->addRow("工号：", workNumberEdit_);
    form->addRow("科室：", departmentEdit_);
    form->addRow("个人资料：", bioEdit_);
    form->addRow("照片：", photoRowW);
    form->addRow("上班开始：", workStartEdit_);
    form->addRow("上班结束：", workEndEdit_);
    form->addRow("挂号费用：", feeEdit_);
    form->addRow("单日上限：", dailyLimitEdit_);

    root->addLayout(form);

    auto* btns = new QHBoxLayout();
    btns->addStretch();
    refreshBtn_ = new QPushButton("刷新", this);
    saveBtn_ = new QPushButton("保存", this);
    btns->addWidget(refreshBtn_);
    btns->addWidget(saveBtn_);
    root->addLayout(btns);
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
