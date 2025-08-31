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
#include <QButtonGroup>
#include <QRadioButton>
#include <functional>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>

#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/medicalcrudservice.h"
#include "core/services/hospitalizationservice.h"

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
    advicesList_->setFrameShape(QFrame::NoFrame);
    advicesList_->setStyleSheet("background: transparent;");
    advicesList_->setVisible(false); // 默认隐藏，无空白
    adviceLay->addWidget(advicesList_);
    auto* adviceForm = new QHBoxLayout();
    // 类型与优先级：单选按钮组
    adviceTypeGroup_ = new QButtonGroup(adviceBox);
    advicePriorityGroup_ = new QButtonGroup(adviceBox);
    adviceTypeGroup_->setExclusive(true);
    advicePriorityGroup_->setExclusive(true);

    auto makeRadioRow = [](QWidget* parent,
                           const QString& labelText,
                           const QStringList& options,
                           QButtonGroup* group,
                           const std::function<QString(const QString&)>& toDisplay){
        auto* row = new QWidget(parent);
        auto* hl = new QHBoxLayout(row);
        hl->setContentsMargins(0,0,0,0);
        hl->setSpacing(8);
        auto* label = new QLabel(labelText, row);
        label->setStyleSheet("color:#555;");
        hl->addWidget(label);
        int id = 0;
        for (const QString& opt : options) {
            auto* rb = new QRadioButton(toDisplay ? toDisplay(opt) : opt, row);
            rb->setProperty("value", opt); // 保留英文枚举
            group->addButton(rb, id++);
            hl->addWidget(rb);
        }
        hl->addStretch();
        return row;
    };
    adviceContentEdit_ = new QTextEdit(adviceBox); adviceContentEdit_->setPlaceholderText(tr("医嘱内容")); adviceContentEdit_->setFixedHeight(60);
    adviceAddBtn_ = new QPushButton(tr("添加医嘱"), adviceBox);
    auto* adviceLeft = new QVBoxLayout();
    auto toTypeLabel = [this](const QString& en) -> QString {
        if (en == "medication") return tr("用药");
        if (en == "lifestyle") return tr("生活方式");
        if (en == "followup") return tr("随访");
        if (en == "examination") return tr("检查");
        return en;
    };
    auto toPrioLabel = [this](const QString& en) -> QString {
        if (en == "low") return tr("低");
        if (en == "normal") return tr("普通");
        if (en == "high") return tr("高");
        if (en == "urgent") return tr("紧急");
        return en;
    };
    auto* typeRow = makeRadioRow(adviceBox, tr("类型:"), {"medication","lifestyle","followup","examination"}, adviceTypeGroup_, toTypeLabel);
    auto* prioRow = makeRadioRow(adviceBox, tr("优先级:"), {"low","normal","high","urgent"}, advicePriorityGroup_, toPrioLabel);
    auto* typePrioCol = new QVBoxLayout();
    typePrioCol->setSpacing(6);
    typePrioCol->addWidget(typeRow);
    typePrioCol->addWidget(prioRow);
    adviceLeft->addLayout(typePrioCol);
    adviceLeft->addWidget(adviceContentEdit_);
    adviceForm->addLayout(adviceLeft); adviceForm->addWidget(adviceAddBtn_);
    adviceLay->addLayout(adviceForm);
    root->addWidget(adviceBox);

    auto* prescriptionBox = new QGroupBox(tr("处方"), this);
    auto* prescriptionLay = new QVBoxLayout(prescriptionBox);
    prescriptionsList_ = new QListWidget(prescriptionBox);
    prescriptionsList_->setFrameShape(QFrame::NoFrame);
    prescriptionsList_->setStyleSheet("background: transparent;");
    prescriptionsList_->setVisible(false); // 默认隐藏，无空白
    prescriptionLay->addWidget(prescriptionsList_);
    auto* prescriptionForm = new QHBoxLayout();
    prescriptionNotesEdit_ = new QLineEdit(prescriptionBox);
    prescriptionNotesEdit_->setPlaceholderText(tr("处方备注"));
    prescriptionAddBtn_ = new QPushButton(tr("开具处方"), prescriptionBox);
    prescriptionForm->addWidget(prescriptionNotesEdit_);
    prescriptionForm->addWidget(prescriptionAddBtn_);
    prescriptionLay->addLayout(prescriptionForm);
    root->addWidget(prescriptionBox);

    // --- Hospitalization Section ---
    hospService_ = new HospitalizationService(client_, this);
    hospBox_ = new QGroupBox(tr("是否住院"), this);
    auto* hospLay = new QVBoxLayout(hospBox_);
    // Yes/No
    hospYesNoGroup_ = new QButtonGroup(hospBox_);
    auto* ynRow = new QWidget(hospBox_);
    auto* ynHL = new QHBoxLayout(ynRow); ynHL->setContentsMargins(0,0,0,0); ynHL->setSpacing(12);
    auto* rbNo = new QRadioButton(tr("否"), ynRow);
    auto* rbYes = new QRadioButton(tr("是"), ynRow);
    hospYesNoGroup_->addButton(rbNo, 0);
    hospYesNoGroup_->addButton(rbYes, 1);
    rbNo->setChecked(true);
    ynHL->addWidget(new QLabel(tr("住院："), ynRow)); ynHL->addWidget(rbNo); ynHL->addWidget(rbYes); ynHL->addStretch();
    hospLay->addWidget(ynRow);

    // Details form (wrapped in a container, only visible when Yes)
    hospDetails_ = new QWidget(hospBox_);
    auto* detailsLay = new QVBoxLayout(hospDetails_);
    detailsLay->setContentsMargins(0,0,0,0);
    auto* hospForm = new QFormLayout();
    hospWardEdit_ = new QLineEdit(hospDetails_); hospWardEdit_->setPlaceholderText(tr("病房号"));
    hospBedEdit_ = new QLineEdit(hospDetails_); hospBedEdit_->setPlaceholderText(tr("床号"));
    hospAdmissionDateEdit_ = new QDateEdit(hospDetails_); hospAdmissionDateEdit_->setDisplayFormat("yyyy-MM-dd"); hospAdmissionDateEdit_->setCalendarPopup(true); hospAdmissionDateEdit_->setDate(QDate::currentDate());
    hospForm->addRow(tr("病房号："), hospWardEdit_);
    hospForm->addRow(tr("床号："), hospBedEdit_);
    hospForm->addRow(tr("入院日期："), hospAdmissionDateEdit_);
    // 出院日期与日费用已移除
    detailsLay->addLayout(hospForm);
    hospLay->addWidget(hospDetails_);

    auto* hospBtnRow = new QHBoxLayout();
    hospBtnRow->addStretch();
    hospCreateBtn_ = new QPushButton(tr("创建住院记录"), hospBox_);
    hospBtnRow->addWidget(hospCreateBtn_);
    hospLay->addLayout(hospBtnRow);
    // 初始隐藏详情并根据切换显示
    auto setDetailsVisible = [this](bool vis){
        if (hospDetails_) hospDetails_->setVisible(vis);
        if (hospCreateBtn_) hospCreateBtn_->setEnabled(vis);
    };
    setDetailsVisible(false);
    // 直接监听“是”按钮
    QObject::connect(rbYes, &QRadioButton::toggled, this, [setDetailsVisible](bool checked){
        setDetailsVisible(checked);
    });
    // 初始化一次
    setDetailsVisible(rbYes->isChecked());
    root->addWidget(hospBox_);

    connect(hospService_, &HospitalizationService::created, this, [this](bool ok, const QString& err){
        if (ok) QMessageBox::information(this, tr("成功"), tr("住院记录已创建"));
        else QMessageBox::warning(this, tr("失败"), err.isEmpty() ? tr("创建失败") : err);
    });
    connect(hospCreateBtn_, &QPushButton::clicked, this, [this]{
        if (hospYesNoGroup_->checkedId() != 1) return; // 只有选择“是”才创建
        // 基础校验
    const QString ad = hospAdmissionDateEdit_->date().isValid() ? hospAdmissionDateEdit_->date().toString("yyyy-MM-dd") : QString();
    if (ad.isEmpty()) { QMessageBox::warning(this, tr("提示"), tr("请选择入院日期")); return; }
        QJsonObject data;
        data["patient_username"] = appt_.value("patient_username").toString();
        data["doctor_username"] = doctorUsername_;
        data["admission_date"] = ad;
    data["discharge_date"] = ""; // 不填写出院日期
        data["ward"] = hospWardEdit_->text().trimmed();
        data["bed_number"] = hospBedEdit_->text().trimmed();
    data["daily_cost"] = 0.0; // 不填写日费用
        data["total_cost"] = 0.0;
        data["status"] = "admitted";
        hospService_->create(data);
    });

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
        if (rows.isEmpty()) {
            advicesList_->setVisible(false);
        } else {
            for (const auto& v : rows) {
                const auto a = v.toObject();
                advicesList_->addItem(QString("[%1|%2] %3").arg(a.value("advice_type").toString(), a.value("priority").toString(), a.value("content").toString()));
            }
            advicesList_->setVisible(true);
        }
    });
    connect(service_, &MedicalCrudService::adviceCreated, this, [this](bool ok, const QString& msg){
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("医嘱已成功添加！"));
            requestAdvices();
            adviceContentEdit_->clear();
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("医嘱添加失败：%1").arg(msg));
        }
    });
    connect(service_, &MedicalCrudService::prescriptionsFetched, this, [this](const QJsonArray& rows){
        prescriptionsList_->clear();
        if (rows.isEmpty()) {
            prescriptionsList_->setVisible(false);
        } else {
            for (const auto& v : rows) {
                const auto p = v.toObject();
                prescriptionsList_->addItem(QString("[%1] %2 - %3").arg(
                    p.value("prescription_date").toString(),
                    p.value("status").toString(),
                    p.value("notes").toString()));
            }
            prescriptionsList_->setVisible(true);
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
    QString adviceType;
    if (auto* btn = adviceTypeGroup_->checkedButton()) adviceType = btn->property("value").toString();
    QString priority;
    if (auto* btn = advicePriorityGroup_->checkedButton()) priority = btn->property("value").toString();
    if (adviceType.isEmpty() || priority.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请选择医嘱类型与优先级"));
        return;
    }
    QJsonObject data{{"record_id", recordId_},
                    {"advice_type", adviceType},
                    {"content", adviceContentEdit_->toPlainText()},
                    {"priority", priority}};
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
