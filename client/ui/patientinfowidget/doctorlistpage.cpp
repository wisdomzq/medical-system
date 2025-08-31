#include "doctorlistpage.h"
#include "doctorinfopage.h"
#include "core/network/communicationclient.h"
#include "core/services/doctorlistservice.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QJsonDocument>
#include <QDebug>

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
    m_mainLayout->setContentsMargins(15, 15, 15, 15);
    m_mainLayout->setSpacing(10);
    
    // 标题
    QLabel* titleLabel = new QLabel("医生信息查询");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
    m_mainLayout->addWidget(titleLabel);
    
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
    m_searchButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #3498db;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #2980b9;"
        "}"
    );
    
    m_searchLayout->addWidget(searchLabel);
    m_searchLayout->addWidget(m_searchEdit);
    m_searchLayout->addWidget(deptLabel);
    m_searchLayout->addWidget(m_departmentCombo);
    m_searchLayout->addWidget(m_searchButton);
    m_searchLayout->addStretch();
    
    m_mainLayout->addLayout(m_searchLayout);
    
    // 医生列表表格
    m_doctorTable = new QTableWidget();
    m_doctorTable->setColumnCount(6);
    QStringList headers = {"姓名", "科室", "职称", "专业", "挂号费", "操作"};
    m_doctorTable->setHorizontalHeaderLabels(headers);
    
    // 设置表格样式
    m_doctorTable->setAlternatingRowColors(true);
    m_doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_doctorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_doctorTable->horizontalHeader()->setStretchLastSection(true);
    m_doctorTable->verticalHeader()->setVisible(false);
    
    // 设置列宽
    m_doctorTable->setColumnWidth(0, 100); // 姓名
    m_doctorTable->setColumnWidth(1, 100); // 科室
    m_doctorTable->setColumnWidth(2, 120); // 职称
    m_doctorTable->setColumnWidth(3, 150); // 专业
    m_doctorTable->setColumnWidth(4, 80);  // 挂号费
    
    m_mainLayout->addWidget(m_doctorTable);
    
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
    m_mainLayout->addLayout(buttonLayout);
    
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
        m_doctorTable->setItem(i, 0, new QTableWidgetItem(doctor.value("name").toString()));
        
        // 科室
        m_doctorTable->setItem(i, 1, new QTableWidgetItem(doctor.value("department").toString()));
        
        // 职称
        m_doctorTable->setItem(i, 2, new QTableWidgetItem(doctor.value("title").toString()));
        
        // 专业
        m_doctorTable->setItem(i, 3, new QTableWidgetItem(doctor.value("specialization").toString()));
        
        // 挂号费
        double fee = doctor.value("consultation_fee").toDouble();
        m_doctorTable->setItem(i, 4, new QTableWidgetItem(QString("¥%1").arg(fee, 0, 'f', 2)));
        
        // 操作列 - 存储医生用户名用于后续查询
        QTableWidgetItem* actionItem = new QTableWidgetItem("查看详情");
        actionItem->setData(Qt::UserRole, doctor.value("username").toString());
        m_doctorTable->setItem(i, 5, actionItem);
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
