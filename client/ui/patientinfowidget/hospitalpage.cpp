#include "hospitalpage.h"
#include "core/network/communicationclient.h"
#include "core/services/hospitalizationservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QJsonArray>
#include <QDateTime>

HospitalPage::HospitalPage(CommunicationClient *c,const QString &patient,QWidget *parent)
    : BasePage(c,patient,parent) {
    QVBoxLayout *layout=new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    
    // 顶部栏（仿照医生端风格）
    QWidget* topBar = new QWidget(this);
    topBar->setObjectName("hospitalTopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    QLabel* title = new QLabel("住院信息", topBar);
    title->setObjectName("hospitalTitle");
    QLabel* subTitle = new QLabel("住院记录查询", topBar);
    subTitle->setObjectName("hospitalSubTitle");
    QVBoxLayout* titleV = new QVBoxLayout();
    titleV->setContentsMargins(0,0,0,0);
    titleV->addWidget(title);
    titleV->addWidget(subTitle);
    topBarLayout->addLayout(titleV);
    topBarLayout->addStretch();
    layout->addWidget(topBar);
    
    // 内容区域
    QWidget* contentWidget = new QWidget(this);
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(15, 15, 15, 15);
    contentLayout->setSpacing(10);
    
    QHBoxLayout *bar=new QHBoxLayout; refreshBtn=new QPushButton("刷新"); filterDoctorEdit=new QLineEdit; filterDoctorEdit->setPlaceholderText("按医生过滤(可选)"); bar->addWidget(filterDoctorEdit); bar->addStretch(); bar->addWidget(refreshBtn); contentLayout->addLayout(bar);
    table=new QTableWidget; table->setColumnCount(8); table->setHorizontalHeaderLabels({"ID","主治医生","病房","床号","就诊日期","入院日期","出院日期","状态"}); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows); contentLayout->addWidget(table); connect(refreshBtn,&QPushButton::clicked,this,&HospitalPage::refresh);
    
    layout->addWidget(contentWidget);
    // 服务化
    m_service = new HospitalizationService(m_client, this);
    connect(m_service, &HospitalizationService::fetched, this, [this](const QJsonArray& arr){
    table->setRowCount(arr.size()); int r=0; for(const auto &v: arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ table->setItem(r,c,new QTableWidgetItem(t)); }; QString dd = o.value("discharge_date").toString(); if(dd.trimmed().isEmpty()) dd = "未出院"; 
    // 格式化就诊日期和入院日期
    QString visitDate = o.value("admission_date").toString();
    QString admissionDate = o.value("admission_date").toString();
    if (visitDate.length() > 10 && visitDate.contains(" ")) {
        QDateTime dt = QDateTime::fromString(visitDate, "yyyy-MM-dd hh:mm:ss");
        if (dt.isValid()) {
            visitDate = dt.toString("yyyy-MM-dd-hh:mm");
        }
    }
    if (admissionDate.length() > 10 && admissionDate.contains(" ")) {
        QDateTime dt = QDateTime::fromString(admissionDate, "yyyy-MM-dd hh:mm:ss");
        if (dt.isValid()) {
            admissionDate = dt.toString("yyyy-MM-dd-hh:mm");
        }
    }
    set(0,QString::number(o.value("id").toInt())); set(1,o.value("doctor_username").toString()); set(2,o.value("ward").toString()); set(3,o.value("bed_number").toString()); set(4,visitDate); set(5,admissionDate); set(6,dd); set(7,o.value("status").toString()); ++r; }
    });
    connect(m_service, &HospitalizationService::fetchFailed, this, [this](const QString&){ /* 可加入状态提示 */ });
    refresh(); }

void HospitalPage::refresh(){ if(filterDoctorEdit->text().trimmed().isEmpty()) m_service->fetchByPatient(m_patientName); else m_service->fetchByDoctor(filterDoctorEdit->text().trimmed()); }

void HospitalPage::handleResponse(const QJsonObject &){ /* 已服务化，兼容入口保留 */ }
