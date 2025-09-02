#include "casepage.h"
#include "core/network/communicationclient.h"
#include "core/services/medicalrecordservice.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QUuid>
#include <QDebug>
#include <QDateTime>

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
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("caseTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("我的病例", topBar);
    title->setObjectName("caseTitle");
    QLabel* subTitle = new QLabel("病例记录与查看", topBar);
    subTitle->setObjectName("caseSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    
    // 刷新按钮
    m_backButton = new QPushButton("刷新");
    m_backButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #ffffff;"
        "    color: #4f63dd;"
        "    border: 1px solid #d9e6fb;"
        "    border-radius: 8px;"
        "    padding: 8px 14px;"
        "    font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #eef2ff;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #e0e7ff;"
        "}"
    );
    connect(m_backButton, &QPushButton::clicked, this, &CasePage::onRefreshButtonClicked);
    
    topBarLayout->addStretch();
    topBarLayout->addWidget(m_backButton);
    m_mainLayout->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);
    
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
    m_recordTable->setColumnWidth(1, 140); // 日期时间（增加宽度）
    m_recordTable->setColumnWidth(2, 100); // 科室
    m_recordTable->setColumnWidth(3, 120); // 医生
    
    // 连接双击信号
    connect(m_recordTable, &QTableWidget::cellDoubleClicked, this, &CasePage::onRowDoubleClicked);
    
    // 添加到主布局
    contentLayout->addWidget(m_recordTable);
    m_mainLayout->addWidget(contentWidget);
}

void CasePage::onConnected()
{
    qDebug() << "[ CasePage ] 客户端已连接，加载病例数据";
    loadMedicalRecords();
}

// 响应由服务处理

void CasePage::onRefreshButtonClicked()
{
    // 刷新病例数据
    qDebug() << "[ CasePage ] 刷新病例数据";
    loadMedicalRecords();
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
        qDebug() << "[ CasePage ] 客户端未初始化，无法加载病例数据";
        return;
    }
    
    m_service->fetchByPatient(m_patientName);
}

void CasePage::populateTable(const QJsonArray &records)
{
    m_records = records;
    m_recordTable->setRowCount(records.size());
    
    for (int i = 0; i < records.size(); ++i) {
        QJsonObject record = records[i].toObject();
        
        // 序号
        m_recordTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        
        // 日期 - 显示完整的日期时间
        QString date = record.value("visit_date").toString();
        // 如果日期格式是YYYY-MM-DD HH:MM:SS，显示年月日-时刻
        if (date.length() > 10 && date.contains(" ")) {
            QDateTime dt = QDateTime::fromString(date, "yyyy-MM-dd hh:mm:ss");
            if (dt.isValid()) {
                date = dt.toString("yyyy-MM-dd-hh:mm"); // 显示年月日-时刻
            }
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
    
    qDebug() << "[ CasePage ] 表格已填充，共" << records.size() << "条记录";
}
