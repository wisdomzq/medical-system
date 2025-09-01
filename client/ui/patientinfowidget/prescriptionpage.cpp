#include "prescriptionpage.h"
#include "core/network/communicationclient.h"
#include "core/services/prescriptionservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QTextEdit>
#include <QGroupBox>
#include <QSplitter>
#include <QJsonDocument>

PrescriptionPage::PrescriptionPage(CommunicationClient *c, const QString &patient, QWidget *parent)
    : BasePage(c, patient, parent) {
    setupUI();
    
    // 服务化
    m_service = new PrescriptionService(m_client, this);
    connect(m_service, &PrescriptionService::listFetched, this, [this](const QJsonArray& data){ populateTable(data); m_statusLabel->setText(QString("已加载 %1 条处方记录").arg(data.size())); });
    connect(m_service, &PrescriptionService::listFailed, this, [this](const QString& err){ m_statusLabel->setText(QString("加载失败: %1").arg(err)); QMessageBox::warning(this, "错误", QString("获取处方列表失败:\n%1").arg(err)); });
    connect(m_service, &PrescriptionService::detailsFetched, this, [this](const QJsonObject& data){ showPrescriptionDetails(data); m_statusLabel->setText("处方详情已显示"); });
    connect(m_service, &PrescriptionService::detailsFailed, this, [this](const QString& err){ m_statusLabel->setText(QString("获取详情失败: %1").arg(err)); QMessageBox::warning(this, "错误", QString("获取处方详情失败:\n%1").arg(err)); });
    
    // 初始加载数据
    requestPrescriptionList();
}

void PrescriptionPage::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    
    // 标题和统计信息
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel("<h2>我的处方</h2>");
    m_countLabel = new QLabel("处方数量: 0");
    m_countLabel->setStyleSheet("color: #666; font-size: 14px;");
    
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_countLabel);
    mainLayout->addLayout(headerLayout);
    
    // 操作按钮
    auto *buttonLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("刷新列表");
    m_detailsBtn = new QPushButton("查看详情");
    m_detailsBtn->setEnabled(false);
    
    connect(m_refreshBtn, &QPushButton::clicked, this, &PrescriptionPage::refreshList);
    connect(m_detailsBtn, &QPushButton::clicked, this, &PrescriptionPage::onDetailsClicked);
    
    buttonLayout->addWidget(m_refreshBtn);
    buttonLayout->addWidget(m_detailsBtn);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    
    // 处方列表表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    QStringList headers = {"序号", "日期", "科室", "主治医生", "状态", "总金额(元)"};
    m_table->setHorizontalHeaderLabels(headers);
    
    // 设置表格属性
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    
    // 设置列宽
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setColumnWidth(0, 60);   // 序号
    m_table->setColumnWidth(1, 120);  // 日期
    m_table->setColumnWidth(2, 100);  // 科室
    m_table->setColumnWidth(3, 120);  // 医生
    m_table->setColumnWidth(4, 80);   // 状态
    
    // 连接表格信号
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this](){
        m_selectedRow = m_table->currentRow();
        m_detailsBtn->setEnabled(m_selectedRow >= 0);
    });
    
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &PrescriptionPage::onTableDoubleClick);
    
    mainLayout->addWidget(m_table);
    
    // 状态标签
    m_statusLabel = new QLabel("准备就绪");
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    mainLayout->addWidget(m_statusLabel);
}

void PrescriptionPage::populateTable(const QJsonArray &prescriptions) {
    m_prescriptions = prescriptions;
    m_table->setRowCount(prescriptions.size());
    
    for (int i = 0; i < prescriptions.size(); ++i) {
        QJsonObject prescription = prescriptions[i].toObject();
        
        // 序号
        auto *seqItem = new QTableWidgetItem(QString::number(prescription.value("sequence").toInt()));
        seqItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, seqItem);
        
        // 日期 (格式化显示)
        QString dateStr = prescription.value("prescription_date").toString();
        if (dateStr.contains("T")) {
            dateStr = dateStr.split("T")[0]; // 只取日期部分
        }
        auto *dateItem = new QTableWidgetItem(dateStr);
        dateItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, dateItem);
        
        // 科室
        auto *deptItem = new QTableWidgetItem(prescription.value("department").toString());
        deptItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, deptItem);
        
        // 主治医生
        QString doctorName = prescription.value("doctor_name").toString();
        QString doctorTitle = prescription.value("doctor_title").toString();
        QString doctorDisplay = doctorTitle.isEmpty() ? doctorName : QString("%1 (%2)").arg(doctorName, doctorTitle);
        auto *doctorItem = new QTableWidgetItem(doctorDisplay);
        m_table->setItem(i, 3, doctorItem);
        
        // 状态
        QString status = prescription.value("status").toString();
        QString statusText;
        if (status == "pending") statusText = "待配药";
        else if (status == "dispensed") statusText = "已配药";
        else if (status == "cancelled") statusText = "已取消";
        else statusText = status;
        
        auto *statusItem = new QTableWidgetItem(statusText);
        statusItem->setTextAlignment(Qt::AlignCenter);
        
        // 设置状态颜色
        if (status == "pending") statusItem->setForeground(QColor("#FFA500")); // 橙色
        else if (status == "dispensed") statusItem->setForeground(QColor("#008000")); // 绿色
        else if (status == "cancelled") statusItem->setForeground(QColor("#FF0000")); // 红色
        
        m_table->setItem(i, 4, statusItem);
        
        // 总金额
        double amount = prescription.value("total_amount").toDouble();
        auto *amountItem = new QTableWidgetItem(QString("¥%1").arg(QString::number(amount, 'f', 2)));
        amountItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(i, 5, amountItem);
        
        // 保存处方ID到第一列的用户数据中
        seqItem->setData(Qt::UserRole, prescription.value("id").toInt());
    }
    
    // 更新计数
    m_countLabel->setText(QString("处方数量: %1").arg(prescriptions.size()));
    
    // 调整行高
    m_table->resizeRowsToContents();
}

void PrescriptionPage::showPrescriptionDetails(const QJsonObject &details) {
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("处方详情");
    dialog->setMinimumSize(800, 600);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    auto *layout = new QVBoxLayout(dialog);
    
    // 处方基本信息
    auto *infoGroup = new QGroupBox("处方信息");
    auto *infoLayout = new QVBoxLayout(infoGroup);
    
    QString infoHtml = QString(R"(
        <table style="width: 100%; border-collapse: collapse;">
        <tr><td style="padding: 5px; font-weight: bold; width: 120px;">处方编号:</td><td style="padding: 5px;">%1</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">开具日期:</td><td style="padding: 5px;">%2</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">科室:</td><td style="padding: 5px;">%3</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">主治医生:</td><td style="padding: 5px;">%4 (%5)</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">患者:</td><td style="padding: 5px;">%6</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">状态:</td><td style="padding: 5px;">%7</td></tr>
        <tr><td style="padding: 5px; font-weight: bold;">总金额:</td><td style="padding: 5px; color: #FF6600; font-weight: bold;">¥%8</td></tr>
        </table>
    )").arg(
        QString::number(details.value("id").toInt()),
        details.value("prescription_date").toString().split("T")[0],
        details.value("department").toString(),
        details.value("doctor_name").toString(),
        details.value("doctor_title").toString(),
        details.value("patient_name").toString(),
        details.value("status_text").toString(),
        QString::number(details.value("total_amount").toDouble(), 'f', 2)
    );
    
    auto *infoLabel = new QLabel(infoHtml);
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    layout->addWidget(infoGroup);
    
    // 药品详情
    auto *itemsGroup = new QGroupBox("药品详情");
    auto *itemsLayout = new QVBoxLayout(itemsGroup);
    
    auto *itemsTable = new QTableWidget();
    QJsonArray items = details.value("items").toArray();
    itemsTable->setRowCount(items.size());
    itemsTable->setColumnCount(7);
    QStringList itemHeaders = {"药品名称", "数量", "剂量", "用法", "疗程", "单价", "小计"};
    itemsTable->setHorizontalHeaderLabels(itemHeaders);
    
    for (int i = 0; i < items.size(); ++i) {
        QJsonObject item = items[i].toObject();
        
        itemsTable->setItem(i, 0, new QTableWidgetItem(item.value("medication_name").toString()));
        itemsTable->setItem(i, 1, new QTableWidgetItem(QString("%1 %2")
            .arg(item.value("quantity").toInt())
            .arg(item.value("unit").toString())));
        itemsTable->setItem(i, 2, new QTableWidgetItem(item.value("dosage").toString()));
        itemsTable->setItem(i, 3, new QTableWidgetItem(item.value("frequency").toString()));
        itemsTable->setItem(i, 4, new QTableWidgetItem(item.value("duration").toString()));
        itemsTable->setItem(i, 5, new QTableWidgetItem(QString("¥%1")
            .arg(QString::number(item.value("unit_price").toDouble(), 'f', 2))));
        itemsTable->setItem(i, 6, new QTableWidgetItem(QString("¥%1")
            .arg(QString::number(item.value("total_price").toDouble(), 'f', 2))));
        
        // 如果有用药说明，添加到工具提示
        QString instructions = item.value("instructions").toString();
        if (!instructions.isEmpty()) {
            itemsTable->item(i, 0)->setToolTip(QString("用药说明: %1").arg(instructions));
        }
    }
    
    // 设置表格占满容器宽度
    itemsTable->horizontalHeader()->setStretchLastSection(true);
    
    // 设置各列的相对宽度比例
    QHeaderView* header = itemsTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);      // 药品名称 - 自适应剩余空间
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // 数量 - 根据内容调整
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // 剂量 - 根据内容调整  
    header->setSectionResizeMode(3, QHeaderView::Fixed);        // 用法 - 固定宽度
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // 疗程 - 根据内容调整
    header->setSectionResizeMode(5, QHeaderView::Fixed);        // 单价 - 固定宽度
    header->setSectionResizeMode(6, QHeaderView::Fixed);        // 小计 - 固定宽度
    
    // 为固定宽度的列设置合适的宽度
    itemsTable->setColumnWidth(3, 120); // 用法列
    itemsTable->setColumnWidth(5, 100); // 单价列
    itemsTable->setColumnWidth(6, 100); // 小计列
    
    itemsTable->setAlternatingRowColors(true);
    itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    itemsTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    itemsTable->setMinimumHeight(150);
    
    // 确保表格水平填充整个容器
    itemsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    itemsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    itemsLayout->addWidget(itemsTable);
    layout->addWidget(itemsGroup);
    
    // 备注信息
    QString notes = details.value("notes").toString();
    if (!notes.isEmpty()) {
        auto *notesGroup = new QGroupBox("备注");
        auto *notesLayout = new QVBoxLayout(notesGroup);
        auto *notesLabel = new QLabel(notes);
        notesLabel->setWordWrap(true);
        notesLabel->setStyleSheet("padding: 10px; background-color: #f5f5f5; border-radius: 5px;");
        notesLayout->addWidget(notesLabel);
        layout->addWidget(notesGroup);
    }
    
    // 关闭按钮
    auto *buttonLayout = new QHBoxLayout();
    auto *closeBtn = new QPushButton("关闭");
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);
    layout->addLayout(buttonLayout);
    
    dialog->exec();
}

void PrescriptionPage::refreshList() {
    m_statusLabel->setText("正在刷新处方列表...");
    m_selectedRow = -1;
    m_detailsBtn->setEnabled(false);
    requestPrescriptionList();
}

void PrescriptionPage::onDetailsClicked() {
    if (m_selectedRow < 0 || m_selectedRow >= m_prescriptions.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个处方");
        return;
    }
    
    QJsonObject prescription = m_prescriptions[m_selectedRow].toObject();
    int prescriptionId = prescription.value("id").toInt();
    
    m_statusLabel->setText("正在获取处方详情...");
    requestPrescriptionDetails(prescriptionId);
}

void PrescriptionPage::onTableDoubleClick(int row, int column) {
    Q_UNUSED(column)
    if (row >= 0 && row < m_prescriptions.size()) {
        m_selectedRow = row;
        onDetailsClicked();
    }
}

void PrescriptionPage::requestPrescriptionList() {
    m_service->fetchList(m_patientName);
}

void PrescriptionPage::requestPrescriptionDetails(int prescriptionId) {
    m_service->fetchDetails(prescriptionId);
}

void PrescriptionPage::sendJson(const QJsonObject &) { /* 已服务化，不再直发 */ }

void PrescriptionPage::handleResponse(const QJsonObject &) { /* 已服务化，兼容入口保留为空实现 */ }
