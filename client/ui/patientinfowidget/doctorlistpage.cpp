#include "doctorlistpage.h"
#include "doctorinfopage.h"
#include "core/network/communicationclient.h"
#include "core/services/doctorlistservice.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QDebug>
#include <QTime>

namespace {
// 与预约页一致：统一从 workTime/working_days/title 中解析工作时间
static QString extractWorkTime(const QJsonObject& o) {
    const QString wt = o.value("workTime").toString();
    if (!wt.isEmpty()) return wt;

    const QString workingDays = o.value("working_days").toString();

    QString timeRange;
    const QString title = o.value("title").toString();
    if (!title.isEmpty()) {
        const auto parts = title.split('-');
        if (parts.size() == 2) {
            const QTime s = QTime::fromString(parts[0].trimmed(), "HH:mm");
            const QTime e = QTime::fromString(parts[1].trimmed(), "HH:mm");
            if (s.isValid() && e.isValid()) timeRange = s.toString("HH:mm") + "-" + e.toString("HH:mm");
        }
    }

    if (!workingDays.isEmpty() && !timeRange.isEmpty()) return workingDays + " " + timeRange;
    if (!workingDays.isEmpty()) return workingDays;
    if (!timeRange.isEmpty()) return timeRange;
    return title; // 兜底
}
}

DoctorListPage::DoctorListPage(CommunicationClient* client, const QString& patientName, QWidget *parent)
    : BasePage(client, patientName, parent), m_doctorInfoPage(nullptr)
{
    setupUI();
    m_service = new DoctorListService(m_client, this);
    connect(m_service, &DoctorListService::fetched, this, [this](const QJsonArray& doctors){ m_doctors = doctors; displayDoctors(doctors); });
    connect(m_service, &DoctorListService::failed, this, [this](const QString& err){ QMessageBox::warning(this, "错误", QString("获取医生列表失败\n%1").arg(err)); });
    loadDoctorList();
}

void DoctorListPage::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("doctorListTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("医生信息查询", topBar);
    title->setObjectName("doctorListTitle");
    QLabel* subTitle = new QLabel("查看医生详细信息", topBar);
    subTitle->setObjectName("doctorListSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    topBarLayout->addStretch();
    m_mainLayout->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(15, 15, 15, 15);
    contentLayout->setSpacing(10);
    
    // 搜索区域
    m_searchLayout = new QHBoxLayout();
    
    QLabel* searchLabel = new QLabel("搜索:");
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("输入医生姓名或专业...");
    
    QLabel* deptLabel = new QLabel("科室:");
    m_departmentCombo = new QComboBox();
    m_departmentCombo->addItem("全部科室", "");
    m_departmentCombo->addItem("内科", "内科");
    m_departmentCombo->addItem("外科", "外科");
    m_departmentCombo->addItem("儿科", "儿科");
    m_departmentCombo->addItem("妇产科", "妇产科");
    m_departmentCombo->addItem("眼科", "眼科");
    m_departmentCombo->addItem("耳鼻喉科", "耳鼻喉科");
    m_departmentCombo->addItem("皮肤科", "皮肤科");
    
    m_searchButton = new QPushButton("搜索");
    m_searchButton->setObjectName("primaryBtn");
    
    m_searchLayout->addWidget(searchLabel);
    m_searchLayout->addWidget(m_searchEdit);
    m_searchLayout->addWidget(deptLabel);
    m_searchLayout->addWidget(m_departmentCombo);
    m_searchLayout->addWidget(m_searchButton);
    m_searchLayout->addStretch();
    
    contentLayout->addLayout(m_searchLayout);
    
    // 医生列表表格
    m_doctorTable = new QTableWidget();
    m_doctorTable->setColumnCount(6);
    QStringList headers = {"姓名", "科室", "工作时间", "专业", "挂号费", "操作"};
    m_doctorTable->setHorizontalHeaderLabels(headers);
    // 表头居中与高度
    m_doctorTable->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_doctorTable->horizontalHeader()->setFixedHeight(36);
    
    // 设置表格样式
    m_doctorTable->setAlternatingRowColors(true);
    m_doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_doctorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_doctorTable->horizontalHeader()->setStretchLastSection(true);
    m_doctorTable->verticalHeader()->setVisible(false);
    m_doctorTable->setFocusPolicy(Qt::NoFocus);
    m_doctorTable->setWordWrap(false);
    m_doctorTable->setStyleSheet(
        "QTableWidget {"
        "  background-color: #ffffff;"
        "  border: 1px solid #e5e7eb;"
        "  border-radius: 8px;"
        "  gridline-color: #f1f5f9;"
        "}"
        "QTableWidget::item {"
        "  padding: 6px;"
        "}"
        "QTableWidget::item:!selected:alternate {"
        "  background: #f9fafb;"
        "}"
        "QTableWidget::item:selected {"
        "  background-color: #eef2ff;"
        "  color: #1f2937;"
        "}"
        "QHeaderView::section {"
        "  background: #f8fafc;"
        "  padding: 8px;"
        "  border: none;"
        "  border-right: 1px solid #e5e7eb;"
        "  font-weight: 600;"
        "}"
        "QHeaderView::section:last {"
        "  border-right: none;"
        "}"
        "QTableCornerButton::section {"
        "  background: #f8fafc;"
        "  border: none;"
        "}"
    );
    
    // 设置列宽
    m_doctorTable->setColumnWidth(0, 100); // 姓名
    m_doctorTable->setColumnWidth(1, 100); // 科室
    m_doctorTable->setColumnWidth(2, 160); // 工作时间
    m_doctorTable->setColumnWidth(3, 150); // 专业
    m_doctorTable->setColumnWidth(4, 80);  // 挂号费
    
    contentLayout->addWidget(m_doctorTable);
    
    // 操作按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    m_viewButton = new QPushButton("查看详细信息");
    m_viewButton->setEnabled(false);
    m_viewButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #27ae60;"
        "    color: white;"
        "    border: none;"
        "    padding: 10px 20px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #229954;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #bdc3c7;"
        "}"
    );
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_viewButton);
    contentLayout->addLayout(buttonLayout);
    
    m_mainLayout->addWidget(contentWidget);
    
    // 连接信号
    connect(m_searchButton, &QPushButton::clicked, this, &DoctorListPage::onSearchClicked);
    connect(m_viewButton, &QPushButton::clicked, this, &DoctorListPage::onViewDoctorClicked);
    connect(m_doctorTable, &QTableWidget::itemSelectionChanged, [this]() {
        m_viewButton->setEnabled(m_doctorTable->currentRow() >= 0);
    });
    connect(m_doctorTable, &QTableWidget::cellDoubleClicked, this, &DoctorListPage::onDoctorTableDoubleClicked);
}

void DoctorListPage::loadDoctorList()
{
    m_service->fetchAllDoctors();
}

void DoctorListPage::displayDoctors(const QJsonArray& doctors)
{
    m_doctorTable->setRowCount(doctors.size());
    
    for (int i = 0; i < doctors.size(); ++i) {
        QJsonObject doctor = doctors[i].toObject();
        
        // 姓名
        {
            auto* item = new QTableWidgetItem(doctor.value("name").toString());
            item->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 0, item);
        }
        
        // 科室
        {
            auto* item = new QTableWidgetItem(doctor.value("department").toString());
            item->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 1, item);
        }
        
        // 工作时间（working_days + title 中的 HH:mm-HH:mm）
        {
            const auto work = extractWorkTime(doctor);
            auto* item = new QTableWidgetItem(work);
            if (work.length() > 40) item->setToolTip(work);
            item->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 2, item);
        }
        
        // 专业
        {
            const auto spec = doctor.value("specialization").toString();
            auto* item = new QTableWidgetItem(spec);
            if (spec.length() > 50) item->setToolTip(spec);
            item->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 3, item);
        }
        
        // 挂号费
        double fee = doctor.value("consultation_fee").toDouble();
        {
            auto* item = new QTableWidgetItem(QString("¥%1").arg(fee, 0, 'f', 2));
            item->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 4, item);
        }
        
        // 操作列 - 存储医生用户名用于后续查询
        {
            auto* actionItem = new QTableWidgetItem("查看详情");
            actionItem->setData(Qt::UserRole, doctor.value("username").toString());
            actionItem->setTextAlignment(Qt::AlignCenter);
            m_doctorTable->setItem(i, 5, actionItem);
        }

        // 行高
        m_doctorTable->setRowHeight(i, 40);
    }
}

void DoctorListPage::onSearchClicked()
{
    QString keyword = m_searchEdit->text();
    QString department = m_departmentCombo->currentData().toString();
    
    // 过滤医生列表
    QJsonArray filteredDoctors;
    for (const QJsonValue& doctorValue : m_doctors) {
        QJsonObject doctor = doctorValue.toObject();
        
        bool matchKeyword = keyword.isEmpty() || 
                           doctor.value("name").toString().contains(keyword, Qt::CaseInsensitive) ||
                           doctor.value("specialization").toString().contains(keyword, Qt::CaseInsensitive);
        
        bool matchDepartment = department.isEmpty() || 
                              doctor.value("department").toString() == department;
        
        if (matchKeyword && matchDepartment) {
            filteredDoctors.append(doctor);
        }
    }
    
    displayDoctors(filteredDoctors);
}

void DoctorListPage::onViewDoctorClicked()
{
    int currentRow = m_doctorTable->currentRow();
    if (currentRow >= 0) {
        QTableWidgetItem* item = m_doctorTable->item(currentRow, 5);
        if (item) {
            QString doctorUsername = item->data(Qt::UserRole).toString();
            openDoctorInfo(doctorUsername);
        }
    }
}

void DoctorListPage::onDoctorTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row >= 0) {
        QTableWidgetItem* item = m_doctorTable->item(row, 5);
        if (item) {
            QString doctorUsername = item->data(Qt::UserRole).toString();
            openDoctorInfo(doctorUsername);
        }
    }
}

void DoctorListPage::openDoctorInfo(const QString& doctorUsername)
{
    if (m_doctorInfoPage) {
        m_doctorInfoPage->close();
        m_doctorInfoPage->deleteLater();
    }
    
    // 创建独立的医生信息窗口（不传递父窗口）
    m_doctorInfoPage = new DoctorInfoPage(doctorUsername, m_client, nullptr);
    connect(m_doctorInfoPage, &DoctorInfoPage::closeRequested, this, &DoctorListPage::onDoctorInfoClosed);
    
    // 显示窗口并置于前台
    m_doctorInfoPage->show();
    m_doctorInfoPage->raise();
    m_doctorInfoPage->activateWindow();
}

void DoctorListPage::onDoctorInfoClosed()
{
    if (m_doctorInfoPage) {
        m_doctorInfoPage->deleteLater();
        m_doctorInfoPage = nullptr;
    }
}
