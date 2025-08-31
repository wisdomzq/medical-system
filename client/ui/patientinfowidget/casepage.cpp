#include "casepage.h"
#include "core/network/communicationclient.h"
#include "core/services/medicalrecordservice.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QUuid>
#include <QDebug>

CasePage::CasePage(CommunicationClient *client, const QString &patientName, QWidget *parent)
    : BasePage(client, patientName, parent)
    , m_mainLayout(nullptr)
    , m_headerLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_backButton(nullptr)
    , m_recordTable(nullptr)
{
    setupUI();
    
    if (m_client) {
        connect(m_client, &CommunicationClient::connected, this, &CasePage::onConnected);
        m_service = new MedicalRecordService(m_client, this);
        connect(m_service, &MedicalRecordService::fetched, this, [this](const QJsonArray& records){ m_records = records; populateTable(records); });
        connect(m_service, &MedicalRecordService::fetchFailed, this, [this](const QString& err){ QMessageBox::warning(this, "错误", "获取病例失败：" + err); });
        loadMedicalRecords();
    }
}

void CasePage::setupUI()
{
    // 主布局
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(20, 20, 20, 20);
    m_mainLayout->setSpacing(15);
    
    // 头部布局
    m_headerLayout = new QHBoxLayout();
    
    // 标题
    m_titleLabel = new QLabel("我的病例");
    m_titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    
    // 刷新按钮
    m_refreshButton = new QPushButton("刷新病例");
    m_refreshButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #229954;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1e8449;"
        "}"
    );
    connect(m_refreshButton, &QPushButton::clicked, this, &CasePage::loadMedicalRecords);
    
    // 返回按钮
    m_backButton = new QPushButton("返回");
    m_backButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #21618c;"
        "}"
    );
    connect(m_backButton, &QPushButton::clicked, this, &CasePage::onBackButtonClicked);
    
    m_headerLayout->addWidget(m_titleLabel);
    m_headerLayout->addStretch();
    m_headerLayout->addWidget(m_refreshButton);
    m_headerLayout->addWidget(m_backButton);
    
    // 病例表格
    m_recordTable = new QTableWidget();
    m_recordTable->setColumnCount(5);
    
    QStringList headers;
    headers << "序号" << "就诊日期" << "科室" << "主治医生" << "诊断结果";
    m_recordTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格样式
    m_recordTable->setAlternatingRowColors(true);
    m_recordTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_recordTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_recordTable->horizontalHeader()->setStretchLastSection(true);
    m_recordTable->verticalHeader()->setVisible(false);
    
    // 设置列宽
    m_recordTable->setColumnWidth(0, 60);  // 序号
    m_recordTable->setColumnWidth(1, 120); // 日期
    m_recordTable->setColumnWidth(2, 100); // 科室
    m_recordTable->setColumnWidth(3, 120); // 医生
    
    // 连接双击信号
    connect(m_recordTable, &QTableWidget::cellDoubleClicked, this, &CasePage::onRowDoubleClicked);
    
    // 添加到主布局
    m_mainLayout->addLayout(m_headerLayout);
    m_mainLayout->addWidget(m_recordTable);
}

void CasePage::onConnected()
{
    qDebug() << "[CasePage] 客户端已连接，加载病例数据";
    loadMedicalRecords();
}

// 响应由服务处理

void CasePage::onBackButtonClicked()
{
    // 发送返回上一页的信号
    emit backRequested();
}

void CasePage::onRowDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    
    if (row < 0 || row >= m_records.size()) {
        return;
    }
    
    QJsonObject record = m_records[row].toObject();
    int recordId = record.value("id").toInt();
    QString date = record.value("visit_date").toString();
    QString department = record.value("department").toString();
    QString doctor = record.value("doctor_name").toString();
    QString diagnosis = record.value("diagnosis").toString();
    
    // 显示详细信息对话框
    QString details = QString(
        "病例详情\n\n"
        "就诊日期：%1\n"
        "科室：%2\n"
        "主治医生：%3\n"
        "诊断结果：%4"
    ).arg(date, department, doctor, diagnosis);
    
    QMessageBox::information(this, "病例详情", details);
}

void CasePage::loadMedicalRecords()
{
    if (!m_client) {
        qDebug() << "[CasePage] 客户端未初始化，无法加载病例数据";
        return;
    }
    
    m_service->fetchByPatient(m_patientName);
}

void CasePage::populateTable(const QJsonArray &records)
{
    m_recordTable->setRowCount(records.size());
    
    for (int i = 0; i < records.size(); ++i) {
        QJsonObject record = records[i].toObject();
        
        // 序号
        m_recordTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        
        // 日期
        QString date = record.value("visit_date").toString();
        if (date.contains(" ")) {
            date = date.split(" ")[0]; // 只显示日期部分
        }
        m_recordTable->setItem(i, 1, new QTableWidgetItem(date));
        
        // 科室
        m_recordTable->setItem(i, 2, new QTableWidgetItem(record.value("department").toString()));
        
        // 主治医生
        QString doctorName = record.value("doctor_name").toString();
        QString doctorTitle = record.value("doctor_title").toString();
        if (!doctorTitle.isEmpty()) {
            doctorName += " (" + doctorTitle + ")";
        }
        m_recordTable->setItem(i, 3, new QTableWidgetItem(doctorName));
        
        // 诊断结果
        QString diagnosis = record.value("diagnosis").toString();
        if (diagnosis.length() > 50) {
            diagnosis = diagnosis.left(47) + "...";
        }
        m_recordTable->setItem(i, 4, new QTableWidgetItem(diagnosis));
    }
    
    qDebug() << "[CasePage] 表格已填充，共" << records.size() << "条记录";
}
