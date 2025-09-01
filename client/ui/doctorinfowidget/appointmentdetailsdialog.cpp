#include "appointmentdetailsdialog.h"
#include "medicationselectiondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QJsonArray>
#include <QGroupBox>
#include <QMessageBox>
#include <QDateTime>
#include <QButtonGroup>
#include <QRadioButton>
#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <functional>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>

#include "core/network/communicationclient.h"
#include "core/network/protocol.h"
#include "core/services/medicalcrudservice.h"
#include "core/services/hospitalizationservice.h"
#include "core/services/appointmentservice.h"

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
    completeBtn_ = new QPushButton(tr("完成诊断"), this);
    completeBtn_->setEnabled(false); // 默认禁用，开具处方后启用
    btnRow->addWidget(completeBtn_);
    root->addLayout(btnRow);

    auto* adviceBox = new QGroupBox(tr("医嘱"), this);
    auto* adviceLay = new QVBoxLayout(adviceBox);
    advicesList_ = new QListWidget(adviceBox);
    advicesList_->setFrameShape(QFrame::NoFrame);
    advicesList_->setStyleSheet("background: transparent; margin: 0; padding: 0;");
    advicesList_->setVisible(false); // 始终隐藏
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
    adviceAddBtn_->setEnabled(false); // 初始禁用，待 recordId_ 就绪再启用
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
    prescriptionsList_->setStyleSheet("background: transparent; margin: 0; padding: 0;");
    prescriptionsList_->setVisible(false); // 始终隐藏
    prescriptionLay->addWidget(prescriptionsList_);
    
    // 处方药品表格
    setupPrescriptionItemsTable();
    prescriptionLay->addWidget(prescriptionItemsTable_);
    
    // 药品操作按钮
    auto* medicationBtnLayout = new QHBoxLayout();
    addMedicationBtn_ = new QPushButton(tr("添加药品"), prescriptionBox);
    removeMedicationBtn_ = new QPushButton(tr("删除药品"), prescriptionBox);
    removeMedicationBtn_->setEnabled(false);
    medicationBtnLayout->addWidget(addMedicationBtn_);
    medicationBtnLayout->addWidget(removeMedicationBtn_);
    medicationBtnLayout->addStretch();
    prescriptionLay->addLayout(medicationBtnLayout);
    
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
    connect(completeBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onCompleteDiagnosis);
    
    // 处方药品相关连接
    connect(addMedicationBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onAddMedication);
    connect(removeMedicationBtn_, &QPushButton::clicked, this, &AppointmentDetailsDialog::onRemoveMedication);

    service_ = new MedicalCrudService(client_, this);
    apptService_ = new AppointmentService(client_, this);
    connect(apptService_, &AppointmentService::statusUpdated, this, [this](bool ok, int apptId, const QString& status, const QString& err){
        if (ok) {
            QMessageBox::information(this, tr("成功"), tr("该预约已标记为完成"));
            emit diagnosisCompleted(apptId);
            accept(); // 关闭对话框
        } else {
            QMessageBox::warning(this, tr("失败"), err.isEmpty() ? tr("更新状态失败") : err);
        }
    });
    connect(service_, &MedicalCrudService::recordsFetched, this, [this](const QJsonArray& arr){
        const int targetApptId = appt_.value("id").toInt();
        const QString targetDate = appt_.value("appointment_date").toString();
        int fallbackByDateId = -1;
        QJsonObject fallbackRec;
        for (const auto& v : arr) {
            const auto rec = v.toObject();
            // 优先: 通过 appointment_id 精确匹配
            if (rec.contains("appointment_id") && rec.value("appointment_id").toInt() == targetApptId) {
                recordId_ = rec.value("id").toInt();
                // 尝试拉取完整详情
                service_->getRecordDetails(recordId_, appt_.value("patient_username").toString());
                if (adviceAddBtn_) adviceAddBtn_->setEnabled(true);
                break;
            }
            // 兼容后端精简返回：无 appointment_id 时，回退按日期匹配
            if (rec.value("visit_date").toString() == targetDate) {
                fallbackByDateId = rec.value("id").toInt();
                fallbackRec = rec;
            }
        }
        if (recordId_ < 0 && fallbackByDateId > 0) {
            recordId_ = fallbackByDateId;
            service_->getRecordDetails(recordId_, appt_.value("patient_username").toString());
            if (adviceAddBtn_) adviceAddBtn_->setEnabled(true);
        }
        requestAdvices();
    });
    connect(service_, &MedicalCrudService::recordDetailsFetched, this, [this](const QJsonObject& full){
        // 用完整详情填充所有字段
        populateFromRecord(full);
    });
    connect(service_, &MedicalCrudService::recordCreated, this, [this](bool ok, const QString& msg, int rid){
        if (ok) {
            if (isSaving_) {
                QMessageBox::information(this, tr("保存成功"), tr("病例记录已成功创建！"));
            }
            if (rid > 0) {
                recordId_ = rid;
                if (adviceAddBtn_) adviceAddBtn_->setEnabled(true);
                requestAdvices();
            } else {
                // 未返回 record_id，回退获取一次以拿到 recordId_
                requestExistingRecord();
            }
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("病例记录创建失败：%1").arg(msg));
        }
        isSaving_ = false;
        if (saveBtn_) saveBtn_->setEnabled(true);
    });
    connect(service_, &MedicalCrudService::recordUpdated, this, [this](bool ok, const QString& msg){
        if (ok) {
            if (isSaving_) {
                QMessageBox::information(this, tr("保存成功"), tr("病例记录已成功更新！"));
            }
            if (adviceAddBtn_) adviceAddBtn_->setEnabled(true);
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("病例记录更新失败：%1").arg(msg));
        }
        isSaving_ = false;
        if (saveBtn_) saveBtn_->setEnabled(true);
    });
    connect(service_, &MedicalCrudService::advicesFetched, this, [this](const QJsonArray& /*rows*/){
        // 需求：上方不要显示文字 -> 始终隐藏列表
        advicesList_->clear();
        advicesList_->setVisible(false);
    });
    connect(service_, &MedicalCrudService::adviceCreated, this, [this](bool ok, const QString& msg){
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("医嘱已成功添加！"));
            requestAdvices();
            // 保留输入内容，不清空
        } else {
            QMessageBox::warning(this, tr("保存失败"), tr("医嘱添加失败：%1").arg(msg));
        }
    });
    connect(service_, &MedicalCrudService::prescriptionsFetched, this, [this](const QJsonArray& rows){
        // 需求：上方不要显示文字 -> 始终隐藏列表
        prescriptionsList_->clear();
        prescriptionsList_->setVisible(false);
        // 若已有与当前 recordId_ 关联的处方，则允许“完成诊断”
        if (recordId_ > 0) {
            for (const auto& v : rows) {
                const auto obj = v.toObject();
                if (obj.value("record_id").toInt() == recordId_) {
                    if (completeBtn_) completeBtn_->setEnabled(true);
                    break;
                }
            }
        }
    });
    connect(service_, &MedicalCrudService::prescriptionCreated, this, [this](bool ok, const QString& msg){
        qDebug() << "处方创建结果:" << ok << "消息:" << msg;
        if (ok) {
            QMessageBox::information(this, tr("保存成功"), tr("处方已成功开具！"));
            requestPrescriptions();
            // 像医嘱一样，保留输入内容，不清空
            if (completeBtn_) completeBtn_->setEnabled(true); // 处方已开具后允许完成诊断
        } else {
            QString errorMsg = msg.isEmpty() ? tr("未知错误") : msg;
            QMessageBox::warning(this, tr("保存失败"), tr("处方开具失败：%1").arg(errorMsg));
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
    // 仅在字段存在时才覆盖，避免精简响应清空本地输入
    if (record.contains("chief_complaint"))
        chiefEdit_->setText(record.value("chief_complaint").toString());
    if (record.contains("diagnosis"))
        diagnosisEdit_->setPlainText(record.value("diagnosis").toString());
    if (record.contains("treatment_plan"))
        planEdit_->setPlainText(record.value("treatment_plan").toString());
    if (record.contains("notes"))
        notesEdit_->setPlainText(record.value("notes").toString());
}

// 由 service 信号驱动，已移除直接解析

void AppointmentDetailsDialog::onSaveRecord() {
    if (isSaving_) return;
    isSaving_ = true;
    if (saveBtn_) saveBtn_->setEnabled(false);
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
    
    // 检查是否有处方项
    if (prescriptionItems_.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先添加药品才能开具处方！");
        return;
    }
    
    // 计算总金额
    double totalAmount = 0.0;
    for (const auto& itemValue : prescriptionItems_) {
        QJsonObject item = itemValue.toObject();
        totalAmount += item.value("total_price").toDouble();
    }
    
    qDebug() << "处方项数量:" << prescriptionItems_.size();
    qDebug() << "总金额:" << totalAmount;
    
    QJsonObject data{
        {"record_id", recordId_},
        {"patient_username", appt_.value("patient_username").toString()},
        {"doctor_username", doctorUsername_},
        {"prescription_date", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")},
        {"total_amount", totalAmount},
        {"status", "pending"},
        {"notes", prescriptionNotesEdit_->text()},
        {"items", prescriptionItems_}
    };
    
    qDebug() << "Sending create_prescription with data:" << data;
    service_->createPrescription(data);
}

void AppointmentDetailsDialog::onCompleteDiagnosis() {
    const int apptId = appt_.value("id").toInt();
    if (apptId <= 0) {
        QMessageBox::warning(this, tr("提示"), tr("预约信息缺失，无法更新状态"));
        return;
    }
    if (QMessageBox::question(this, tr("确认"), tr("确认将该预约标记为已完成吗？")) != QMessageBox::Yes) return;
    if (apptService_) {
        apptService_->updateStatus(apptId, "completed");
    }
}

void AppointmentDetailsDialog::setupPrescriptionItemsTable() {
    prescriptionItemsTable_ = new QTableWidget(0, 6, this);
    QStringList headers = {tr("药品名称"), tr("数量"), tr("剂量"), tr("用法"), tr("疗程"), tr("单价")};
    prescriptionItemsTable_->setHorizontalHeaderLabels(headers);
    prescriptionItemsTable_->horizontalHeader()->setStretchLastSection(true);
    prescriptionItemsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    prescriptionItemsTable_->setAlternatingRowColors(true);
    
    // 设置表格选择变化的信号
    connect(prescriptionItemsTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AppointmentDetailsDialog::onMedicationSelectionChanged);
}

void AppointmentDetailsDialog::updatePrescriptionItemsTable() {
    prescriptionItemsTable_->setRowCount(prescriptionItems_.size());
    
    for (int i = 0; i < prescriptionItems_.size(); ++i) {
        QJsonObject item = prescriptionItems_[i].toObject();
        
        prescriptionItemsTable_->setItem(i, 0, new QTableWidgetItem(item.value("medication_name").toString()));
        QString quantityText = QString("%1 %2").arg(item.value("quantity").toInt()).arg(item.value("unit").toString());
        prescriptionItemsTable_->setItem(i, 1, new QTableWidgetItem(quantityText));
        prescriptionItemsTable_->setItem(i, 2, new QTableWidgetItem(item.value("dosage").toString()));
        prescriptionItemsTable_->setItem(i, 3, new QTableWidgetItem(item.value("frequency").toString()));
        prescriptionItemsTable_->setItem(i, 4, new QTableWidgetItem(item.value("duration").toString()));
        prescriptionItemsTable_->setItem(i, 5, new QTableWidgetItem(QString("¥%1").arg(QString::number(item.value("unit_price").toDouble(), 'f', 2))));
    }
}

void AppointmentDetailsDialog::onMedicationSelectionChanged() {
    bool hasSelection = prescriptionItemsTable_->selectionModel()->hasSelection();
    removeMedicationBtn_->setEnabled(hasSelection);
}

void AppointmentDetailsDialog::onAddMedication() {
    showMedicationSelectionDialog();
}

void AppointmentDetailsDialog::onRemoveMedication() {
    int currentRow = prescriptionItemsTable_->currentRow();
    if (currentRow >= 0 && currentRow < prescriptionItems_.size()) {
        prescriptionItems_.removeAt(currentRow);
        updatePrescriptionItemsTable();
    }
}

void AppointmentDetailsDialog::showMedicationSelectionDialog() {
    MedicationSelectionDialog dialog(client_, this);
    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject selectedMedication = dialog.getSelectedMedication();
        prescriptionItems_.append(selectedMedication);
        updatePrescriptionItemsTable();
    }
}
