#include "medicationselectiondialog.h"
#include "core/network/communicationclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>

MedicationSelectionDialog::MedicationSelectionDialog(CommunicationClient* client, QWidget* parent)
    : QDialog(parent), client_(client) {
    setupUI();
    
    connect(client_, &CommunicationClient::jsonReceived, this, &MedicationSelectionDialog::onMedicationResponse);
    
    loadMedications();
}

void MedicationSelectionDialog::setupUI() {
    setWindowTitle(tr("选择药品"));
    setModal(true);
    resize(800, 600);
    
    auto* layout = new QVBoxLayout(this);
    
    // 搜索区域
    auto* searchLayout = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(tr("搜索药品名称..."));
    searchLayout->addWidget(new QLabel(tr("搜索:")));
    searchLayout->addWidget(searchEdit_);
    layout->addLayout(searchLayout);
    
    // 药品列表表格
    medicationsTable_ = new QTableWidget(this);
    QStringList headers = {tr("药品名称"), tr("通用名"), tr("类别"), tr("厂商"), tr("规格"), tr("单位"), tr("单价")};
    medicationsTable_->setColumnCount(headers.size());
    medicationsTable_->setHorizontalHeaderLabels(headers);
    medicationsTable_->horizontalHeader()->setStretchLastSection(true);
    medicationsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    medicationsTable_->setAlternatingRowColors(true);
    layout->addWidget(medicationsTable_);
    
    // 处方信息输入
    auto* prescriptionGroup = new QGroupBox(tr("处方信息"));
    auto* prescriptionLayout = new QFormLayout(prescriptionGroup);
    
    quantitySpinBox_ = new QSpinBox(this);
    quantitySpinBox_->setMinimum(1);
    quantitySpinBox_->setMaximum(999);
    quantitySpinBox_->setValue(1);
    prescriptionLayout->addRow(tr("数量:"), quantitySpinBox_);
    
    dosageEdit_ = new QLineEdit(this);
    dosageEdit_->setPlaceholderText(tr("如: 50mg"));
    prescriptionLayout->addRow(tr("剂量:"), dosageEdit_);
    
    frequencyComboBox_ = new QComboBox(this);
    frequencyComboBox_->addItems({tr("每日一次"), tr("每日两次"), tr("每日三次"), tr("每日四次"), 
                                 tr("每12小时一次"), tr("每8小时一次"), tr("每6小时一次"), tr("按需服用")});
    prescriptionLayout->addRow(tr("用法:"), frequencyComboBox_);
    
    durationEdit_ = new QLineEdit(this);
    durationEdit_->setPlaceholderText(tr("如: 7天"));
    prescriptionLayout->addRow(tr("疗程:"), durationEdit_);
    
    layout->addWidget(prescriptionGroup);
    
    // 按钮
    auto* buttonLayout = new QHBoxLayout();
    cancelBtn_ = new QPushButton(tr("取消"), this);
    confirmBtn_ = new QPushButton(tr("确认"), this);
    confirmBtn_->setEnabled(false);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelBtn_);
    buttonLayout->addWidget(confirmBtn_);
    layout->addLayout(buttonLayout);
    
    // 连接信号
    connect(searchEdit_, &QLineEdit::textChanged, this, &MedicationSelectionDialog::onSearchTextChanged);
    connect(medicationsTable_, &QTableWidget::cellDoubleClicked, this, &MedicationSelectionDialog::onTableDoubleClick);
    connect(medicationsTable_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
        confirmBtn_->setEnabled(medicationsTable_->selectionModel()->hasSelection());
    });
    connect(confirmBtn_, &QPushButton::clicked, this, &MedicationSelectionDialog::onConfirm);
    connect(cancelBtn_, &QPushButton::clicked, this, &MedicationSelectionDialog::onCancel);
}

void MedicationSelectionDialog::loadMedications() {
    QJsonObject request;
    request["action"] = "get_medications";
    client_->sendJson(request);
}

void MedicationSelectionDialog::onSearchTextChanged() {
    filterMedications();
}

void MedicationSelectionDialog::filterMedications() {
    QString searchText = searchEdit_->text().toLower();
    QJsonArray filtered;
    
    for (const auto& med : medications_) {
        QJsonObject medication = med.toObject();
        QString name = medication.value("name").toString().toLower();
        QString genericName = medication.value("generic_name").toString().toLower();
        
        if (searchText.isEmpty() || name.contains(searchText) || genericName.contains(searchText)) {
            filtered.append(medication);
        }
    }
    
    filteredMedications_ = filtered;
    populateTable(filteredMedications_);
}

void MedicationSelectionDialog::populateTable(const QJsonArray& medications) {
    medicationsTable_->setRowCount(medications.size());
    
    for (int i = 0; i < medications.size(); ++i) {
        QJsonObject med = medications[i].toObject();
        
        medicationsTable_->setItem(i, 0, new QTableWidgetItem(med.value("name").toString()));
        medicationsTable_->setItem(i, 1, new QTableWidgetItem(med.value("generic_name").toString()));
        medicationsTable_->setItem(i, 2, new QTableWidgetItem(med.value("category").toString()));
        medicationsTable_->setItem(i, 3, new QTableWidgetItem(med.value("manufacturer").toString()));
        medicationsTable_->setItem(i, 4, new QTableWidgetItem(med.value("specification").toString()));
        medicationsTable_->setItem(i, 5, new QTableWidgetItem(med.value("unit").toString()));
        medicationsTable_->setItem(i, 6, new QTableWidgetItem(QString("¥%1").arg(QString::number(med.value("price").toDouble(), 'f', 2))));
        
        // 将药品ID存储在第一列的UserRole中
        medicationsTable_->item(i, 0)->setData(Qt::UserRole, med.value("id").toInt());
    }
}

void MedicationSelectionDialog::onMedicationResponse(const QJsonObject& response) {
    if (response.value("type").toString() == "medications_response") {
        if (response.value("success").toBool()) {
            medications_ = response.value("data").toArray();
            filterMedications();
        } else {
            QMessageBox::warning(this, tr("错误"), tr("获取药品列表失败"));
        }
    }
}

void MedicationSelectionDialog::onTableDoubleClick(int row, int column) {
    Q_UNUSED(column)
    medicationsTable_->selectRow(row);
    onConfirm();
}

void MedicationSelectionDialog::onConfirm() {
    int currentRow = medicationsTable_->currentRow();
    if (currentRow < 0 || currentRow >= filteredMedications_.size()) {
        QMessageBox::warning(this, tr("警告"), tr("请选择一个药品"));
        return;
    }
    
    // 验证输入
    if (dosageEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请输入剂量"));
        dosageEdit_->setFocus();
        return;
    }
    
    if (durationEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请输入疗程"));
        durationEdit_->setFocus();
        return;
    }
    
    // 构建选中的药品信息
    QJsonObject medication = filteredMedications_[currentRow].toObject();
    selectedMedication_ = medication;
    selectedMedication_["quantity"] = quantitySpinBox_->value();
    selectedMedication_["dosage"] = dosageEdit_->text();
    selectedMedication_["frequency"] = frequencyComboBox_->currentText();
    selectedMedication_["duration"] = durationEdit_->text();
    
    // 计算总价
    double unitPrice = medication.value("price").toDouble();
    int quantity = quantitySpinBox_->value();
    selectedMedication_["unit_price"] = unitPrice;
    selectedMedication_["total_price"] = unitPrice * quantity;
    selectedMedication_["medication_id"] = medication.value("id").toInt();
    selectedMedication_["medication_name"] = medication.value("name").toString();
    selectedMedication_["unit"] = medication.value("unit").toString();
    selectedMedication_["category"] = medication.value("category").toString();
    
    accept();
}

void MedicationSelectionDialog::onCancel() {
    reject();
}
