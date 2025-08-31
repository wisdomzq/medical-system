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
#include <QComboBox>

#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/medicalcrudservice.h"

AppointmentDetailsDialog::AppointmentDetailsDialog(const QString& doctorUsername, const QJsonObject& appt, CommunicationClient* client, QWidget* parent)
    : QDialog(parent), doctorUsername_(doctorUsername), appt_(appt), client_(client), ownsClient_(false) {
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
    completeBtn_ = new QPushButton(tr("完成诊断"), this);
    completeBtn_->setStyleSheet(
        "QPushButton {"
        "    background-color: #28a745;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #218838;"
        "}"
    );
    btnRow->addWidget(saveBtn_);
    btnRow->addWidget(completeBtn_);
    root->addLayout(btnRow);

    auto* adviceBox = new QGroupBox(tr("医嘱"), this);
    auto* adviceLay = new QVBoxLayout(adviceBox);
    advicesList_ = new QListWidget(adviceBox);
    adviceLay->addWidget(advicesList_);
    auto* adviceForm = new QHBoxLayout();
    
    // 将类型改为下拉框
    adviceTypeCombo_ = new QComboBox(adviceBox);
    adviceTypeCombo_->addItems({"medication", "lifestyle", "followup", "examination", "surgery", "therapy"});
    adviceTypeCombo_->setCurrentText("medication");
    
    // 将优先级改为下拉框  
    advicePriorityCombo_ = new QComboBox(adviceBox);
    advicePriorityCombo_->addItems({"low", "normal", "high", "urgent"});
    advicePriorityCombo_->setCurrentText("normal");
    
    adviceContentEdit_ = new QTextEdit(adviceBox); 
    adviceContentEdit_->setPlaceholderText(tr("医嘱内容")); 
    adviceContentEdit_->setFixedHeight(60);
    adviceAddBtn_ = new QPushButton(tr("添加医嘱"), adviceBox);
    
    auto* adviceLeft = new QVBoxLayout();
    auto* row1 = new QHBoxLayout(); 
    row1->addWidget(new QLabel("类型:"));
    row1->addWidget(adviceTypeCombo_); 
    row1->addWidget(new QLabel("优先级:"));
    row1->addWidget(advicePriorityCombo_);
    adviceLeft->addLayout(row1);
    adviceLeft->addWidget(adviceContentEdit_);
    adviceForm->addLayout(adviceLeft); 
    adviceForm->addWidget(adviceAddBtn_);
    adviceLay->addLayout(adviceForm);
    root->addWidget(adviceBox);

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

    connect(client_, &CommunicationClient::connected, this, &AppointmentDetailsDialog::onConnected);
    connect(saveBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onSaveRecord);
    connect(completeBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onCompleteAppointment);
    connect(adviceAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddAdvice);
    connect(prescriptionAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddPrescription);

    service_ = new MedicalCrudService(client_, this);
    connect(service_, &MedicalCrudService::recordsFetched, this, [this](const QJsonArray& arr){
        for (const auto& v : arr) {
            const auto rec = v.toObject();
            if (rec.value("appointment_id").toInt() == appt_.value("id").toInt()) {
                recordId_ = rec.value("id").toInt();
                populateFromRecord(rec);
                requestAdvices();
                requestPrescriptions();
                return;
            }
        }
        // 如果没有找到匹配的记录，确保recordId_为-1
        recordId_ = -1;
    });
    connect(service_, &MedicalCrudService::recordCreated, this, [this](bool ok, const QString& msg, int rid){
        static int lastRecordId = -1; // 防止重复处理
        if (ok && rid > 0 && rid != lastRecordId) {
            lastRecordId = rid;
            recordId_ = rid;
            QMessageBox::information(this, tr("保存成功"), tr("病历记录已成功创建！"));
            requestAdvices();
            requestPrescriptions();
        } else if (ok && rid <= 0) {
            requestExistingRecord(); // 重新获取记录ID
        } else if (!ok) {
            QMessageBox::warning(this, tr("保存失败"), tr("病历记录创建失败：%1").arg(msg));
        }
    });
    connect(service_, &MedicalCrudService::recordUpdated, this, [this](bool ok, const QString& msg){
        if (ok) QMessageBox::information(this, tr("保存成功"), tr("病例记录已成功更新！"));
        else QMessageBox::warning(this, tr("保存失败"), tr("病例记录更新失败：%1").arg(msg));
    });
    connect(service_, &MedicalCrudService::advicesFetched, this, [this](const QJsonArray& rows){
        advicesList_->clear();
        for (const auto& v : rows) {
            const auto a = v.toObject();
            advicesList_->addItem(QString("[%1|%2] %3").arg(a.value("advice_type").toString(), a.value("priority").toString(), a.value("content").toString()));
        }
    });
    connect(service_, &MedicalCrudService::adviceCreated, this, [this](bool ok, const QString& msg){
        static QString lastAdviceRequestId;
        QString currentRequestId = QString::number(QDateTime::currentMSecsSinceEpoch());
        
        if (ok && lastAdviceRequestId != currentRequestId) {
            lastAdviceRequestId = currentRequestId;
            QMessageBox::information(this, tr("保存成功"), tr("医嘱已成功添加！"));
            requestAdvices();
            adviceTypeCombo_->setCurrentText("medication"); 
            adviceContentEdit_->clear(); 
            advicePriorityCombo_->setCurrentText("normal");
        } else if (!ok) {
            QMessageBox::warning(this, tr("保存失败"), tr("医嘱添加失败：%1").arg(msg));
        }
    });
    connect(service_, &MedicalCrudService::prescriptionsFetched, this, [this](const QJsonArray& rows){
        prescriptionsList_->clear();
        for (const auto& v : rows) {
            const auto p = v.toObject();
            prescriptionsList_->addItem(QString("[%1] %2 - %3").arg(
                p.value("prescription_date").toString(),
                p.value("status").toString(),
                p.value("notes").toString()));
        }
    });
    connect(service_, &MedicalCrudService::prescriptionCreated, this, [this](bool ok, const QString& msg){
        static QString lastPrescriptionRequestId;
        QString currentRequestId = QString::number(QDateTime::currentMSecsSinceEpoch());
        
        if (ok && lastPrescriptionRequestId != currentRequestId) {
            lastPrescriptionRequestId = currentRequestId;
            QMessageBox::information(this, tr("保存成功"), tr("处方已成功开具！"));
            requestPrescriptions();
            prescriptionNotesEdit_->clear();
        } else if (!ok) {
            QMessageBox::warning(this, tr("保存失败"), tr("处方开具失败：%1").arg(msg));
        }
    });
}

void AppointmentDetailsDialog::onConnected() {
    requestExistingRecord();
}

void AppointmentDetailsDialog::requestExistingRecord() {
    // 优先通过预约ID查找病历记录
    service_->getRecordsByAppointment(appt_.value("id").toInt());
}

void AppointmentDetailsDialog::requestAdvices() {
    if (recordId_ < 0) return; // 无记录则跳过
    service_->getAdvicesByRecord(recordId_);
}

void AppointmentDetailsDialog::populateFromRecord(const QJsonObject& record) {
    chiefEdit_->setText(record.value("chief_complaint").toString());
    diagnosisEdit_->setPlainText(record.value("diagnosis").toString());
    planEdit_->setPlainText(record.value("treatment_plan").toString());
    notesEdit_->setPlainText(record.value("notes").toString());
}

// 由 service 信号驱动，已移除直接解析

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
    if (recordId_ < 0) service_->createRecord(data);
    else service_->updateRecord(recordId_, data);
}

void AppointmentDetailsDialog::onAddAdvice() {
    qDebug() << "onAddAdvice called, recordId_:" << recordId_;
    if (recordId_ < 0) {
        qDebug() << "recordId_ < 0, showing warning message";
        QMessageBox::warning(this, "提示", "请先保存病历记录后再添加医嘱！");
        return; // 需先保存病历
    }
    QJsonObject data{{"record_id", recordId_},
                    {"advice_type", adviceTypeCombo_->currentText()},
                    {"content", adviceContentEdit_->toPlainText()},
                    {"priority", advicePriorityCombo_->currentText()}};
    qDebug() << "Sending create_medical_advice with data:" << data;
    service_->createAdvice(data);
}

void AppointmentDetailsDialog::requestPrescriptions() {
    if (recordId_ < 0) return; // 无记录则跳过
    service_->getPrescriptionsByPatient(appt_.value("patient_username").toString());
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
    service_->createPrescription(data);
}

void AppointmentDetailsDialog::onCompleteAppointment() {
    // 确认完成诊断
    int ret = QMessageBox::question(this, "完成诊断", 
                                   "确认完成此次诊断吗？完成后将无法再次编辑。",
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        // 更新预约状态为completed
        updateAppointmentStatus("completed");
    }
}

void AppointmentDetailsDialog::updateAppointmentStatus(const QString& status) {
    QJsonObject request;
    request["action"] = "update_appointment_status";
    request["appointment_id"] = appt_.value("id").toInt();
    request["status"] = status;
    
    // 临时处理响应
    connect(client_, &CommunicationClient::jsonReceived, this, [this, status](const QJsonObject& response) {
        if (response.value("type").toString() == "update_appointment_status_response") {
            if (response.value("success").toBool()) {
                QMessageBox::information(this, "完成", "诊断已完成，预约状态已更新！");
                
                // 禁用所有编辑控件
                chiefEdit_->setReadOnly(true);
                diagnosisEdit_->setReadOnly(true);
                planEdit_->setReadOnly(true);
                notesEdit_->setReadOnly(true);
                adviceTypeCombo_->setEnabled(false);
                advicePriorityCombo_->setEnabled(false);
                adviceContentEdit_->setReadOnly(true);
                prescriptionNotesEdit_->setReadOnly(true);
                saveBtn_->setEnabled(false);
                adviceAddBtn_->setEnabled(false);
                prescriptionAddBtn_->setEnabled(false);
                completeBtn_->setEnabled(false);
                
                // 关闭对话框
                QTimer::singleShot(1500, this, &QDialog::accept);
            } else {
                QMessageBox::warning(this, "错误", "更新预约状态失败：" + response.value("error").toString());
            }
            disconnect(client_, &CommunicationClient::jsonReceived, this, nullptr);
        }
    });
    
    client_->sendJson(request);
}
