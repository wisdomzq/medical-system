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

HospitalPage::HospitalPage(CommunicationClient *c,const QString &patient,QWidget *parent)
    : BasePage(c,patient,parent) {
    QVBoxLayout *layout=new QVBoxLayout(this);
    QHBoxLayout *bar=new QHBoxLayout; refreshBtn=new QPushButton("刷新"); filterDoctorEdit=new QLineEdit; filterDoctorEdit->setPlaceholderText("按医生过滤(可选)"); bar->addWidget(filterDoctorEdit); bar->addStretch(); bar->addWidget(refreshBtn); layout->addLayout(bar);
    table=new QTableWidget; table->setColumnCount(8); table->setHorizontalHeaderLabels({"ID","患者","主治医生","病房号","床号","入院日期","出院日期","状态"}); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows); layout->addWidget(table); connect(refreshBtn,&QPushButton::clicked,this,&HospitalPage::refresh);
    // 服务化
    m_service = new HospitalizationService(m_client, this);
    connect(m_service, &HospitalizationService::fetched, this, [this](const QJsonArray& arr){
        table->setRowCount(arr.size()); int r=0; for(const auto &v: arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ table->setItem(r,c,new QTableWidgetItem(t)); }; set(0,QString::number(o.value("id").toInt())); set(1,o.value("patient_username").toString()); set(2,o.value("doctor_username").toString()); set(3,o.value("ward_number").toString()); set(4,o.value("bed_number").toString()); set(5,o.value("admission_date").toString()); set(6,o.value("discharge_date").toString()); set(7,o.value("status").toString()); ++r; }
    });
    connect(m_service, &HospitalizationService::fetchFailed, this, [this](const QString&){ /* 可加入状态提示 */ });
    refresh(); }

void HospitalPage::refresh(){ if(filterDoctorEdit->text().trimmed().isEmpty()) m_service->fetchByPatient(m_patientName); else m_service->fetchByDoctor(filterDoctorEdit->text().trimmed()); }

void HospitalPage::handleResponse(const QJsonObject &){ /* 已服务化，兼容入口保留 */ }
