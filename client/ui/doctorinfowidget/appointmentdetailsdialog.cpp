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
#include "core/services/medicalcrudservice.h"

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
    ownsClient_ = true;
    connect(client_, &CommunicationClient::connected, this, &AppointmentDetailsDialog::onConnected);
    connect(saveBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onSaveRecord);
    connect(adviceAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddAdvice);
    connect(prescriptionAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddPrescription);

    // Service
    service_ = new MedicalCrudService(client_, this);
    connect(service_, &MedicalCrudService::recordsFetched, this, [this](const QJsonArray& arr){
        for (const auto& v : arr) {
            const auto rec = v.toObject();
            if (rec.value("appointment_id").toInt() == appt_.value("id").toInt()) {
                recordId_ = rec.value("id").toInt();
                populateFromRecord(rec);
                break;
            }
        }
        requestAdvices();
    });
    connect(service_, &MedicalCrudService::recordCreated, this, [this](bool ok, const QString& msg, int rid){
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("病例记录已成功创建！"));
            if (rid > 0) { recordId_ = rid; requestAdvices(); }
            else { requestExistingRecord(); }
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("病例记录创建失败：%1").arg(msg));
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
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("医嘱已成功添加！"));
            requestAdvices();
            adviceTypeEdit_->clear(); adviceContentEdit_->clear(); advicePriorityEdit_->clear();
        } else {
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
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("处方已成功开具！"));
            requestPrescriptions();
            prescriptionNotesEdit_->clear();
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("处方开具失败：%1").arg(msg));
        }
    });
    client_->connectToServer("127.0.0.1", Protocol::SERVER_PORT);
}

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
    btnRow->addWidget(saveBtn_);
    root->addLayout(btnRow);

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
    connect(adviceAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddAdvice);
    connect(prescriptionAddBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddPrescription);

    service_ = new MedicalCrudService(client_, this);
    connect(service_, &MedicalCrudService::recordsFetched, this, [this](const QJsonArray& arr){
        for (const auto& v : arr) {
            const auto rec = v.toObject();
            if (rec.value("appointment_id").toInt() == appt_.value("id").toInt()) {
                recordId_ = rec.value("id").toInt();
                populateFromRecord(rec);
                break;
            }
        }
        requestAdvices();
    });
    connect(service_, &MedicalCrudService::recordCreated, this, [this](bool ok, const QString& msg, int rid){
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("病例记录已成功创建！"));
            if (rid > 0) { recordId_ = rid; requestAdvices(); }
            else { requestExistingRecord(); }
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("病例记录创建失败：%1").arg(msg));
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
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("医嘱已成功添加！"));
            requestAdvices();
            adviceTypeEdit_->clear(); adviceContentEdit_->clear(); advicePriorityEdit_->clear();
        } else {
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
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("处方已成功开具！"));
            requestPrescriptions();
            prescriptionNotesEdit_->clear();
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("处方开具失败：%1").arg(msg));
        }
    });
}

void AppointmentDetailsDialog::onConnected() {
    requestExistingRecord();
    requestAdvices();
    requestPrescriptions();
}

void AppointmentDetailsDialog::requestExistingRecord() {
    service_->getRecordsByPatient(appt_.value("patient_username").toString());
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
                    {"advice_type", adviceTypeEdit_->text()},
                    {"content", adviceContentEdit_->toPlainText()},
                    {"priority", advicePriorityEdit_->text()}};
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
