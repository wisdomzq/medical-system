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
#include <QMessageBox>
#include <QBrush>
#include <QColor>

HospitalPage::HospitalPage(CommunicationClient *c,const QString &patient,QWidget *parent)
    : BasePage(c,patient,parent) {
    QVBoxLayout *layout=new QVBoxLayout(this);
    
    // 顶部控制栏
    QHBoxLayout *bar=new QHBoxLayout; 
    filterDoctorEdit=new QLineEdit; 
    filterDoctorEdit->setPlaceholderText("按医生过滤(可选)"); 
    QPushButton *filterBtn = new QPushButton("过滤");
    refreshBtn=new QPushButton("刷新"); 
    QPushButton *clearBtn = new QPushButton("清空");
    
    bar->addWidget(new QLabel("医生筛选:"));
    bar->addWidget(filterDoctorEdit); 
    bar->addWidget(filterBtn);
    bar->addWidget(clearBtn);
    bar->addStretch(); 
    bar->addWidget(refreshBtn); 
    layout->addLayout(bar);
    table=new QTableWidget; 
    table->setColumnCount(8); 
    table->setHorizontalHeaderLabels({"ID","患者","主治医生","病房号","床号","入院日期","出院日期","状态"}); 
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); 
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); 
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    layout->addWidget(table); 
    
    // 连接按钮事件
    connect(refreshBtn,&QPushButton::clicked,this,&HospitalPage::refresh);
    connect(filterBtn, &QPushButton::clicked, this, &HospitalPage::refresh);
    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        filterDoctorEdit->clear();
        refresh();
    });
    connect(filterDoctorEdit, &QLineEdit::returnPressed, this, &HospitalPage::refresh);
    // 服务化
    m_service = new HospitalizationService(m_client, this);
    connect(m_service, &HospitalizationService::fetched, this, [this](const QJsonArray& arr){
        table->setRowCount(arr.size());
        if (arr.isEmpty()) {
            // 显示无数据提示
            QTableWidgetItem* emptyItem = new QTableWidgetItem("暂无住院记录");
            emptyItem->setTextAlignment(Qt::AlignCenter);
            table->setRowCount(1);
            table->setSpan(0, 0, 1, 8);
            table->setItem(0, 0, emptyItem);
            return;
        }
        
        int r = 0;
        for (const auto &v : arr) {
            QJsonObject o = v.toObject();
            auto set = [&](int c, const QString &t) {
                table->setItem(r, c, new QTableWidgetItem(t));
            };
            set(0, QString::number(o.value("id").toInt()));
            set(1, o.value("patient_username").toString());
            set(2, QString("%1 (%2)").arg(o.value("doctor_name").toString(), o.value("doctor_username").toString()));
            set(3, o.value("ward_number").toString());
            set(4, o.value("bed_number").toString());
            set(5, o.value("admission_date").toString());
            set(6, o.value("discharge_date").toString().isEmpty() ? "未出院" : o.value("discharge_date").toString());
            
            // 根据状态设置颜色
            QString status = o.value("status").toString();
            QTableWidgetItem* statusItem = new QTableWidgetItem(status == "admitted" ? "住院中" : 
                                                               status == "discharged" ? "已出院" : status);
            if (status == "admitted") {
                statusItem->setBackground(QBrush(QColor(255, 235, 59, 100))); // 黄色背景
            } else if (status == "discharged") {
                statusItem->setBackground(QBrush(QColor(76, 175, 80, 100))); // 绿色背景
            }
            table->setItem(r, 7, statusItem);
            ++r;
        }
    });
    connect(m_service, &HospitalizationService::fetchFailed, this, [this](const QString& err){
        QMessageBox::warning(this, "错误", "获取住院信息失败：" + (err.isEmpty() ? "请检查网络连接" : err));
        // 显示错误状态
        table->setRowCount(1);
        QTableWidgetItem* errorItem = new QTableWidgetItem("加载失败，请点击刷新重试");
        errorItem->setTextAlignment(Qt::AlignCenter);
        errorItem->setBackground(QBrush(QColor(244, 67, 54, 100))); // 红色背景
        table->setSpan(0, 0, 1, 8);
        table->setItem(0, 0, errorItem);
    });
    refresh(); }

void HospitalPage::refresh(){ if(filterDoctorEdit->text().trimmed().isEmpty()) m_service->fetchByPatient(m_patientName); else m_service->fetchByDoctor(filterDoctorEdit->text().trimmed()); }

void HospitalPage::handleResponse(const QJsonObject &){ /* 已服务化，兼容入口保留 */ }
