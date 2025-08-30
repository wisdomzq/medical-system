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
#include <QMessageBox>
#include <QDateTime>

#include "core/network/communicationclient.h"
#include "core/network/protocol.h"

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

    // prescriptions panel
    auto* prescriptionBox = new QGroupBox(tr("处方"), this);
    auto* prescriptionLay = new QVBoxLayout(prescriptionBox);
    prescriptionsList_ = new QListWidget(prescriptionBox);
    prescriptionLay->addWidget(prescriptionsList_);
    auto* prescriptionForm = new QHBoxLayout();
    prescriptionNotesEdit_ = new QLineEdit(prescriptionBox); 
    prescriptionNotesEdit_->setPlaceholderText(tr("处方备注"));
    prescriptionAddBtn_ = new QPushButton(tr("开具处方"), prescriptionBox);
    prescriptionForm->addWidget(prescriptionNotesEdit_);
    prescriptionForm->addWidget(prescriptionAddBtn_);
    prescriptionLay->addLayout(prescriptionForm);
    root->addWidget(prescriptionBox);

    client_ = new CommunicationClient(this);
    connect(client_, &CommunicationClient::connected, this, &AppointmentDetailsDialog::onConnected);
    connect(client_, &CommunicationClient::jsonReceived, this, &AppointmentDetailsDialog::onJsonReceived);
    connect(saveBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onSaveRecord);
    connect(adviceAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddAdvice);
    connect(prescriptionAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddPrescription);
    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

void AppointmentDetailsDialog::onConnected() {
    requestExistingRecord();
    requestAdvices();
    requestPrescriptions();
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
            QMessageBox::information(this, "保存成功", "病例记录已成功创建！");
            if (obj.contains("record_id") && obj.value("record_id").toInt() > 0) {
                recordId_ = obj.value("record_id").toInt();
                requestAdvices();
            } else {
                // 无返回ID，则重新拉取以定位新建的记录
                requestExistingRecord();
            }
        } else {
            QMessageBox::warning(this, "保存失败", "病例记录创建失败：" + obj.value("message").toString());
        }
    } else if (type == "update_medical_record_response") {
        if (obj.value("success").toBool()) {
            QMessageBox::information(this, "保存成功", "病例记录已成功更新！");
        } else {
            QMessageBox::warning(this, "保存失败", "病例记录更新失败：" + obj.value("message").toString());
        }
    } else if (type == "medical_advices_response") {
        advicesList_->clear();
        for (const auto& v : obj.value("data").toArray()) {
            const auto a = v.toObject();
            advicesList_->addItem(QString("[%1|%2] %3").arg(a.value("advice_type").toString(), a.value("priority").toString(), a.value("content").toString()));
        }
    } else if (type == "create_medical_advice_response") {
        qDebug() << "Received create_medical_advice_response:" << obj;
        if (obj.value("success").toBool()) {
            QMessageBox::information(this, "保存成功", "医嘱已成功添加！");
            requestAdvices();
            // 清空医嘱输入框
            adviceTypeEdit_->clear();
            adviceContentEdit_->clear();
            advicePriorityEdit_->clear();
        } else {
            QMessageBox::warning(this, "保存失败", "医嘱添加失败：" + obj.value("message").toString());
        }
    } else if (type == "prescriptions_response") {
        prescriptionsList_->clear();
        for (const auto& v : obj.value("data").toArray()) {
            const auto p = v.toObject();
            prescriptionsList_->addItem(QString("[%1] %2 - %3").arg(
                p.value("prescription_date").toString(),
                p.value("status").toString(),
                p.value("notes").toString()
            ));
        }
    } else if (type == "create_prescription_response") {
        qDebug() << "Received create_prescription_response:" << obj;
        if (obj.value("success").toBool()) {
            QMessageBox::information(this, "保存成功", "处方已成功开具！");
            requestPrescriptions();
            // 清空处方输入框
            prescriptionNotesEdit_->clear();
        } else {
            QMessageBox::warning(this, "保存失败", "处方开具失败：" + obj.value("message").toString());
        }
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
    qDebug() << "onAddAdvice called, recordId_:" << recordId_;
    if (recordId_ < 0) {
        qDebug() << "recordId_ < 0, showing warning message";
        QMessageBox::warning(this, "提示", "请先保存病历记录后再添加医嘱！");
        return; // 需先保存病历
    }
    QJsonObject data{{"record_id", recordId_},
                    {"advice_type", adviceTypeEdit_->text()},
                    {"content", adviceContentEdit_->toPlainText()},
                    {"priority", advicePriorityEdit_->text()}};
    qDebug() << "Sending create_medical_advice with data:" << data;
    QJsonObject req{{"action", "create_medical_advice"}, {"data", data}};
    client_->sendJson(req);
}

void AppointmentDetailsDialog::requestPrescriptions() {
    if (recordId_ < 0) return; // 无记录则跳过
    QJsonObject req{{"action", "get_prescriptions_by_patient"}, {"patient_username", appt_.value("patient_username").toString()}};
    client_->sendJson(req);
}

void AppointmentDetailsDialog::onAddPrescription() {
    qDebug() << "onAddPrescription called, recordId_:" << recordId_;
    if (recordId_ < 0) {
        QMessageBox::warning(this, "提示", "请先保存病历记录后再开具处方！");
        return;
    }
    
    QJsonObject data{
        {"record_id", recordId_},
        {"patient_username", appt_.value("patient_username").toString()},
        {"doctor_username", doctorUsername_},
        {"prescription_date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"total_amount", 0.0},
        {"status", "pending"},
        {"notes", prescriptionNotesEdit_->text()}
    };
    
    qDebug() << "Sending create_prescription with data:" << data;
    QJsonObject req{{"action", "create_prescription"}, {"data", data}};
    client_->sendJson(req);
}
