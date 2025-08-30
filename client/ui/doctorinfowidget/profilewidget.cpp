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
#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"

ProfileWidget::ProfileWidget(const QString& doctorName, QWidget* parent)
    : QWidget(parent), doctorName_(doctorName)
{
    // 复用登录建立的单例/全局通信器在 LoginWidget 外不可直接拿，这里新建一个客户端用于该页
    client_ = new CommunicationClient(this);
    connect(client_, &CommunicationClient::connected, this, &ProfileWidget::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &ProfileWidget::onJsonReceived);
    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);

    auto* root = new QVBoxLayout(this);
    auto* title = new QLabel(QString("个人信息 - %1").arg(doctorName_));
    title->setStyleSheet("font-size:18px;font-weight:600;margin:4px 0 12px 0;");
    root->addWidget(title);

    // 表单
    auto* form = new QFormLayout();
    workNumberEdit_ = new QLineEdit(this);
    departmentEdit_ = new QLineEdit(this);
    bioEdit_ = new QTextEdit(this);
    bioEdit_->setPlaceholderText("个人资料……");

    // 照片
    photoPreview_ = new QLabel(this);
    photoPreview_->setFixedSize(96,96);
    photoPreview_->setStyleSheet("border:1px solid #ccc;background:#fafafa;");
    auto* choosePhotoBtn = new QPushButton("选择照片", this);

    auto* photoRowW = new QWidget(this);
    auto* photoRow = new QHBoxLayout(photoRowW);
    photoRow->addWidget(photoPreview_);
    photoRow->addWidget(choosePhotoBtn);
    photoRow->addStretch();

    // 上下班时间
    workStartEdit_ = new QTimeEdit(this); workStartEdit_->setDisplayFormat("HH:mm");
    workEndEdit_   = new QTimeEdit(this); workEndEdit_->setDisplayFormat("HH:mm");

    // 费用与上限
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

    // 按钮
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
}

void ProfileWidget::onConnected() { requestProfile(); }

void ProfileWidget::requestProfile() {
    QJsonObject req; req["action"] = "get_doctor_info"; req["username"] = doctorName_;
    client_->sendJson(req);
}

void ProfileWidget::onRefresh() { requestProfile(); }

void ProfileWidget::onJsonReceived(const QJsonObject& resp) {
    const auto type = resp.value("type").toString();
    if (type == "doctor_info_response") {
        if (!resp.value("success").toBool()) return;
        const auto d = resp.value("data").toObject();
        // DB 字段映射
        workNumberEdit_->setText(d.value("work_number").toString());
        departmentEdit_->setText(d.value("department").toString());
        bioEdit_->setPlainText(d.value("specialization").toString()); // 以 specialization 作为“个人资料”
        feeEdit_->setValue(d.value("consultation_fee").toDouble());
        dailyLimitEdit_->setValue(d.value("max_patients_per_day").toInt());
        parseWorkTitle(d.value("title").toString());
        // 照片：当前数据库无图片列，这里仅保留预览占位；可后续扩展存储方案
    } else if (type == "update_doctor_info_response") {
        if (resp.value("success").toBool()) {
            QMessageBox::information(this, "提示", "保存成功");
        } else {
            QMessageBox::warning(this, "失败", "保存失败");
        }
    }
}

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
    // 照片暂不入库（数据库当前无此列）；如需保存，可扩展 doctors 表新增列 photo BLOB。

    QJsonObject req; req["action"] = "update_doctor_info"; req["username"] = doctorName_; req["data"] = data;
    client_->sendJson(req);
}
