#include "advicepage.h"
#include "core/network/communicationclient.h"
#include "core/services/adviceservice.h"
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

AdvicePage::AdvicePage(CommunicationClient *c, const QString &patient, QWidget *parent)
    : BasePage(c, patient, parent) {
    setupUI();
    
    // 服务化
    m_service = new AdviceService(m_client, this);
    connect(m_service, &AdviceService::listFetched, this, [this](const QJsonArray& data){
        populateTable(data);
        m_statusLabel->setText(QString("已加载 %1 条医嘱记录").arg(data.size()));
    });
    connect(m_service, &AdviceService::listFailed, this, [this](const QString& err){
        m_statusLabel->setText(QString("加载失败: %1").arg(err));
        QMessageBox::warning(this, "错误", QString("获取医嘱列表失败:\n%1").arg(err));
    });
    connect(m_service, &AdviceService::detailsFetched, this, [this](const QJsonObject& data){
        showAdviceDetails(data);
        m_statusLabel->setText("医嘱详情已显示");
    });
    connect(m_service, &AdviceService::detailsFailed, this, [this](const QString& err){
        m_statusLabel->setText(QString("获取详情失败: %1").arg(err));
        QMessageBox::warning(this, "错误", QString("获取医嘱详情失败:\n%1").arg(err));
    });
    
    // 初始加载数据
    requestAdviceList();
}

void AdvicePage::setupUI() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("adviceTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("我的医嘱", topBar);
    title->setObjectName("adviceTitle");
    QLabel* subTitle = new QLabel("医嘱管理与查看", topBar);
    subTitle->setObjectName("adviceSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    topBarLayout->addStretch();
    mainLayout->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(12);
    
    // 统计信息
    auto *headerLayout = new QHBoxLayout();
    m_countLabel = new QLabel("医嘱数量: 0");
    m_countLabel->setStyleSheet("color: #666; font-size: 14px;");
    
    headerLayout->addStretch();
    headerLayout->addWidget(m_countLabel);
    contentLayout->addLayout(headerLayout);
    
    // 操作按钮
    auto *buttonLayout = new QHBoxLayout();
    m_refreshBtn = new QPushButton("刷新列表");
    m_detailsBtn = new QPushButton("查看详情");
    m_prescriptionBtn = new QPushButton("查看处方");
    m_detailsBtn->setEnabled(false);
    m_prescriptionBtn->setEnabled(false);
    
    connect(m_refreshBtn, &QPushButton::clicked, this, &AdvicePage::refreshList);
    connect(m_detailsBtn, &QPushButton::clicked, this, &AdvicePage::onDetailsClicked);
    connect(m_prescriptionBtn, &QPushButton::clicked, this, &AdvicePage::onPrescriptionClicked);
    
    buttonLayout->addWidget(m_refreshBtn);
    buttonLayout->addWidget(m_detailsBtn);
    buttonLayout->addWidget(m_prescriptionBtn);
    buttonLayout->addStretch();
    contentLayout->addLayout(buttonLayout);
    
    // 医嘱列表表格
    m_table = new QTableWidget(this);
    m_table->setColumnCount(7);
    QStringList headers = {"序号", "日期", "科室", "主治医生", "医嘱类型", "优先级", "医嘱内容"};
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
    m_table->setColumnWidth(4, 100);  // 医嘱类型
    m_table->setColumnWidth(5, 80);   // 优先级
    
    // 连接表格信号
    connect(m_table, &QTableWidget::itemSelectionChanged, this, [this](){
        m_selectedRow = m_table->currentRow();
        bool hasSelection = m_selectedRow >= 0;
        m_detailsBtn->setEnabled(hasSelection);
        
        // 检查是否有处方
        if (hasSelection && m_selectedRow < m_advices.size()) {
            QJsonObject advice = m_advices[m_selectedRow].toObject();
            bool hasPrescription = advice.value("has_prescription").toBool();
            m_prescriptionBtn->setEnabled(hasPrescription);
        } else {
            m_prescriptionBtn->setEnabled(false);
        }
    });
    
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &AdvicePage::onTableDoubleClick);
    
    contentLayout->addWidget(m_table);
    
    // 状态标签
    m_statusLabel = new QLabel("准备就绪");
    m_statusLabel->setStyleSheet("color: #666; font-size: 12px;");
    contentLayout->addWidget(m_statusLabel);
    
    mainLayout->addWidget(contentWidget);
}

void AdvicePage::populateTable(const QJsonArray &advices) {
    m_advices = advices;
    m_table->setRowCount(advices.size());
    
    for (int i = 0; i < advices.size(); ++i) {
        QJsonObject advice = advices[i].toObject();
        
        // 序号
        auto *seqItem = new QTableWidgetItem(QString::number(advice.value("sequence").toInt()));
        seqItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 0, seqItem);
        
        // 日期 (格式化显示)
        QString dateStr = advice.value("visit_date").toString();
        if (dateStr.contains("T")) {
            dateStr = dateStr.split("T")[0]; // 只取日期部分
        }
        auto *dateItem = new QTableWidgetItem(dateStr);
        dateItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 1, dateItem);
        
        // 科室
        auto *deptItem = new QTableWidgetItem(advice.value("department").toString());
        deptItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, deptItem);
        
        // 主治医生
        QString doctorName = advice.value("doctor_name").toString();
        QString doctorTitle = advice.value("doctor_title").toString();
        QString doctorDisplay = doctorTitle.isEmpty() ? doctorName : QString("%1 (%2)").arg(doctorName, doctorTitle);
        auto *doctorItem = new QTableWidgetItem(doctorDisplay);
        m_table->setItem(i, 3, doctorItem);
        
        // 医嘱类型
        QString adviceTypeText = advice.value("advice_type_text").toString();
        auto *typeItem = new QTableWidgetItem(adviceTypeText);
        typeItem->setTextAlignment(Qt::AlignCenter);
        
        // 设置医嘱类型颜色
        QString adviceType = advice.value("advice_type").toString();
        if (adviceType == "medication") typeItem->setForeground(QColor("#FF6600")); // 橙色
        else if (adviceType == "lifestyle") typeItem->setForeground(QColor("#008000")); // 绿色
        else if (adviceType == "followup") typeItem->setForeground(QColor("#0066CC")); // 蓝色
        else if (adviceType == "examination") typeItem->setForeground(QColor("#9933CC")); // 紫色
        
        m_table->setItem(i, 4, typeItem);
        
        // 优先级
        QString priorityText = advice.value("priority_text").toString();
        auto *priorityItem = new QTableWidgetItem(priorityText);
        priorityItem->setTextAlignment(Qt::AlignCenter);
        
        // 设置优先级颜色
        QString priority = advice.value("priority").toString();
        if (priority == "urgent") priorityItem->setForeground(QColor("#FF0000")); // 红色
        else if (priority == "high") priorityItem->setForeground(QColor("#FF6600")); // 橙色
        else if (priority == "normal") priorityItem->setForeground(QColor("#000000")); // 黑色
        else if (priority == "low") priorityItem->setForeground(QColor("#666666")); // 灰色
        
        m_table->setItem(i, 5, priorityItem);
        
        // 医嘱内容 (显示前50个字符作为摘要)
        QString content = advice.value("content").toString();
        QString contentSummary = content.length() > 50 ? content.left(50) + "..." : content;
        auto *contentItem = new QTableWidgetItem(contentSummary);
        contentItem->setToolTip(content); // 完整内容作为工具提示
        
        m_table->setItem(i, 6, contentItem);
        
        // 保存医嘱ID到第一列的用户数据中
        seqItem->setData(Qt::UserRole, advice.value("id").toInt());
    }
    
    // 更新计数
    m_countLabel->setText(QString("医嘱数量: %1").arg(advices.size()));
    
    // 调整行高
    m_table->resizeRowsToContents();
}

void AdvicePage::showAdviceDetails(const QJsonObject &details) {
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle("医嘱详情");
    dialog->setMinimumSize(700, 500);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    auto *layout = new QVBoxLayout(dialog);
    
    // 医嘱基本信息
    auto *infoGroup = new QGroupBox("医嘱信息");
    auto *infoLayout = new QVBoxLayout(infoGroup);
    
    QString infoHtml = QString(R"(
        <table style="width: 100%; border-collapse: collapse;">
        <tr><td style="padding: 8px; font-weight: bold; width: 120px;">医嘱类型:</td><td style="padding: 8px;">%1</td></tr>
        <tr><td style="padding: 8px; font-weight: bold;">优先级:</td><td style="padding: 8px;">%2</td></tr>
        <tr><td style="padding: 8px; font-weight: bold;">开具日期:</td><td style="padding: 8px;">%3</td></tr>
        <tr><td style="padding: 8px; font-weight: bold;">科室:</td><td style="padding: 8px;">%4</td></tr>
        <tr><td style="padding: 8px; font-weight: bold;">主治医生:</td><td style="padding: 8px;">%5 (%6)</td></tr>
        <tr><td style="padding: 8px; font-weight: bold;">诊断:</td><td style="padding: 8px;">%7</td></tr>
        </table>
    )").arg(
        details.value("advice_type_text").toString(),
        details.value("priority_text").toString(),
        details.value("visit_date").toString().split("T")[0],
        details.value("department").toString(),
        details.value("doctor_name").toString(),
        details.value("doctor_title").toString(),
        details.value("diagnosis").toString()
    );
    
    auto *infoLabel = new QLabel(infoHtml);
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    layout->addWidget(infoGroup);
    
    // 医嘱内容
    auto *contentGroup = new QGroupBox("医嘱内容");
    auto *contentLayout = new QVBoxLayout(contentGroup);
    
    auto *contentText = new QTextEdit();
    contentText->setPlainText(details.value("content").toString());
    contentText->setReadOnly(true);
    contentText->setMaximumHeight(150);
    contentLayout->addWidget(contentText);
    layout->addWidget(contentGroup);
    
    // 处方信息
    bool hasPrescription = details.value("has_prescription").toBool();
    if (hasPrescription) {
        auto *prescriptionGroup = new QGroupBox("相关处方");
        auto *prescriptionLayout = new QVBoxLayout(prescriptionGroup);
        
        auto *prescriptionLabel = new QLabel("该医嘱关联有处方信息");
        prescriptionLabel->setStyleSheet("color: #008000; font-weight: bold;");
        
        auto *viewPrescriptionBtn = new QPushButton("查看处方详情");
        connect(viewPrescriptionBtn, &QPushButton::clicked, [this, details](){
            int prescriptionId = details.value("prescription_id").toInt();
            emit prescriptionRequested(prescriptionId);
        });
        
        prescriptionLayout->addWidget(prescriptionLabel);
        prescriptionLayout->addWidget(viewPrescriptionBtn);
        layout->addWidget(prescriptionGroup);
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

void AdvicePage::refreshList() {
    m_statusLabel->setText("正在刷新医嘱列表...");
    m_selectedRow = -1;
    m_detailsBtn->setEnabled(false);
    m_prescriptionBtn->setEnabled(false);
    requestAdviceList();
}

void AdvicePage::onDetailsClicked() {
    if (m_selectedRow < 0 || m_selectedRow >= m_advices.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个医嘱");
        return;
    }
    
    QJsonObject advice = m_advices[m_selectedRow].toObject();
    int adviceId = advice.value("id").toInt();
    
    // 暂时使用简化版本直接显示详情
    showAdviceDetails(advice);
    
    // TODO: 实现完整的详情获取
    // m_statusLabel->setText("正在获取医嘱详情...");
    // requestAdviceDetails(adviceId);
}

void AdvicePage::onTableDoubleClick(int row, int column) {
    Q_UNUSED(column)
    if (row >= 0 && row < m_advices.size()) {
        m_selectedRow = row;
        onDetailsClicked();
    }
}

void AdvicePage::onPrescriptionClicked() {
    if (m_selectedRow < 0 || m_selectedRow >= m_advices.size()) {
        QMessageBox::warning(this, "警告", "请先选择一个医嘱");
        return;
    }
    
    QJsonObject advice = m_advices[m_selectedRow].toObject();
    bool hasPrescription = advice.value("has_prescription").toBool();
    
    if (!hasPrescription) {
        QMessageBox::information(this, "提示", "该医嘱没有关联的处方");
        return;
    }
    
    int prescriptionId = advice.value("prescription_id").toInt();
    emit prescriptionRequested(prescriptionId);
}

void AdvicePage::requestAdviceList() {
    m_service->fetchAdviceList(m_patientName);
}

void AdvicePage::requestAdviceDetails(int adviceId) {
    m_service->fetchAdviceDetails(adviceId);
}

