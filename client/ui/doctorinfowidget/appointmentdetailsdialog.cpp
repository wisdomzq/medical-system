#include "appointmentdetailsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QJsonArray>
#include <QGroupBox>

#include "core/network/src/client/communicationclient.h"
#include "core/network/src/protocol.h"

AppointmentDetailsDialog::AppointmentDetailsDialog(const QString& doctorUsername, const QJsonObject& appt, QWidget* parent)
    : QDialog(parent), doctorUsername_(doctorUsername), appt_(appt) {
    setWindowTitle(tr("预约详情 - %1").arg(appt_.value("patient_name").toString()));
    resize(720, 520);

    auto* root = new QVBoxLayout(this);
    auto* head = new QLabel(tr("患者：%1  时间：%2 %3")
                                .arg(appt_.value("patient_name").toString())
                                .arg(appt_.value("appointment_date").toString())
                                .arg(appt_.value("appointment_time").toString()), this);
    head->setStyleSheet("font-weight:600;margin-bottom:8px;");
    root->addWidget(head);

    auto* form = new QFormLayout();
    chiefEdit_ = new QLineEdit(this);
    diagnosisEdit_ = new QTextEdit(this);
    planEdit_ = new QTextEdit(this);
    notesEdit_ = new QTextEdit(this);
    form->addRow(tr("主诉"), chiefEdit_);
    form->addRow(tr("诊断"), diagnosisEdit_);
    form->addRow(tr("治疗方案"), planEdit_);
    form->addRow(tr("备注"), notesEdit_);
    root->addLayout(form);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    saveBtn_ = new QPushButton(tr("保存病历"), this);
    btnRow->addWidget(saveBtn_);
    root->addLayout(btnRow);

    // advices panel
    auto* adviceBox = new QGroupBox(tr("医嘱"), this);
    auto* adviceLay = new QVBoxLayout(adviceBox);
    advicesList_ = new QListWidget(adviceBox);
    adviceLay->addWidget(advicesList_);
    auto* adviceForm = new QHBoxLayout();
    adviceTypeEdit_ = new QLineEdit(adviceBox); adviceTypeEdit_->setPlaceholderText(tr("类型: medication/lifestyle/followup/examination"));
    advicePriorityEdit_ = new QLineEdit(adviceBox); advicePriorityEdit_->setPlaceholderText(tr("优先级: low/normal/high/urgent"));
    adviceContentEdit_ = new QTextEdit(adviceBox); adviceContentEdit_->setPlaceholderText(tr("医嘱内容")); adviceContentEdit_->setFixedHeight(60);
    adviceAddBtn_ = new QPushButton(tr("添加医嘱"), adviceBox);
    auto* adviceLeft = new QVBoxLayout();
    auto* row1 = new QHBoxLayout(); row1->addWidget(adviceTypeEdit_); row1->addWidget(advicePriorityEdit_);
    adviceLeft->addLayout(row1);
    adviceLeft->addWidget(adviceContentEdit_);
    adviceForm->addLayout(adviceLeft); adviceForm->addWidget(adviceAddBtn_);
    adviceLay->addLayout(adviceForm);
    root->addWidget(adviceBox);

    client_ = new CommunicationClient(this);
    connect(client_, &CommunicationClient::connected, this, &AppointmentDetailsDialog::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &AppointmentDetailsDialog::onJsonReceived);
    connect(saveBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onSaveRecord);
    connect(adviceAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddAdvice);
    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

void AppointmentDetailsDialog::onConnected() {
    requestExistingRecord();
    requestAdvices();
}

void AppointmentDetailsDialog::requestExistingRecord() {
    QJsonObject req{{"action", "get_medical_records_by_patient"},
                    {"patient_username", appt_.value("patient_username").toString()}};
    client_->sendJson(req);
}

void AppointmentDetailsDialog::requestAdvices() {
    if (recordId_ < 0) return; // 无记录则跳过
    QJsonObject req{{"action", "get_medical_advices_by_record"}, {"record_id", recordId_}};
    client_->sendJson(req);
}

void AppointmentDetailsDialog::populateFromRecord(const QJsonObject& record) {
    chiefEdit_->setText(record.value("chief_complaint").toString());
    diagnosisEdit_->setPlainText(record.value("diagnosis").toString());
    planEdit_->setPlainText(record.value("treatment_plan").toString());
    notesEdit_->setPlainText(record.value("notes").toString());
}

void AppointmentDetailsDialog::onJsonReceived(const QJsonObject& obj) {
    const auto type = obj.value("type").toString();
    if (type == "medical_records_response") {
        const auto arr = obj.value("data").toArray();
        // 尝试查找与本预约匹配的记录（通过 appointment_id）
        for (const auto& v : arr) {
            const auto rec = v.toObject();
            if (rec.value("appointment_id").toInt() == appt_.value("id").toInt()) {
                recordId_ = rec.value("id").toInt();
                populateFromRecord(rec);
                break;
            }
        }
        requestAdvices();
    } else if (type == "create_medical_record_response") {
        if (obj.value("success").toBool()) {
            if (obj.contains("record_id") && obj.value("record_id").toInt() > 0) {
                recordId_ = obj.value("record_id").toInt();
                requestAdvices();
            } else {
                // 无返回ID，则重新拉取以定位新建的记录
                requestExistingRecord();
            }
        }
    } else if (type == "update_medical_record_response") {
        // 可提示成功
    } else if (type == "medical_advices_response") {
        advicesList_->clear();
        for (const auto& v : obj.value("data").toArray()) {
            const auto a = v.toObject();
            advicesList_->addItem(QString("[%1|%2] %3").arg(a.value("advice_type").toString(), a.value("priority").toString(), a.value("content").toString()));
        }
    } else if (type == "create_medical_advice_response") {
        if (obj.value("success").toBool()) requestAdvices();
    }
}

void AppointmentDetailsDialog::onSaveRecord() {
    QJsonObject data{
        {"appointment_id", appt_.value("id").toInt()},
        {"patient_username", appt_.value("patient_username").toString()},
        {"doctor_username", doctorUsername_},
        {"visit_date", appt_.value("appointment_date").toString()},
        {"chief_complaint", chiefEdit_->text()},
        {"present_illness", ""},
        {"past_history", ""},
        {"physical_examination", ""},
        {"diagnosis", diagnosisEdit_->toPlainText()},
        {"treatment_plan", planEdit_->toPlainText()},
        {"notes", notesEdit_->toPlainText()}
    };
    if (recordId_ < 0) {
        QJsonObject req{{"action", "create_medical_record"}, {"data", data}};
        client_->sendJson(req);
    } else {
        QJsonObject req{{"action", "update_medical_record"}, {"record_id", recordId_}, {"data", data}};
        client_->sendJson(req);
    }
}

void AppointmentDetailsDialog::onAddAdvice() {
    if (recordId_ < 0) return; // 需先保存病历
    QJsonObject data{{"record_id", recordId_},
                    {"advice_type", adviceTypeEdit_->text()},
                    {"content", adviceContentEdit_->toPlainText()},
                    {"priority", advicePriorityEdit_->text()}};
    QJsonObject req{{"action", "create_medical_advice"}, {"data", data}};
    client_->sendJson(req);
}
