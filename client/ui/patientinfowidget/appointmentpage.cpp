#include "appointmentpage.h"
#include "core/network/communicationclient.h"
#include "core/services/patientappointmentservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>
#include <QJsonArray>
#include <QDateTime>
#include <QDate>
#include <QTime>

AppointmentPage::AppointmentPage(CommunicationClient *c, const QString &patient, QWidget *parent)
    : BasePage(c, patient, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    // 创建顶部栏
    QWidget *topBar = new QWidget;
    topBar->setObjectName("appointmentTopBar");
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 12, 16, 12);
    
    QLabel *titleLabel = new QLabel("预约挂号");
    titleLabel->setObjectName("appointmentTitle");
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    
    layout->addWidget(topBar);

    // 主内容区域
    QWidget *contentArea = new QWidget;
    QVBoxLayout *contentLayout = new QVBoxLayout(contentArea);
    contentLayout->setContentsMargins(16, 16, 16, 16);
    contentLayout->setSpacing(16);

    // 医生排班卡片
    QWidget *doctorCard = new QWidget;
    doctorCard->setObjectName("doctorCard");
    QVBoxLayout *doctorCardLayout = new QVBoxLayout(doctorCard);
    doctorCardLayout->setContentsMargins(20, 20, 20, 20);
    doctorCardLayout->setSpacing(16);

    // 医生排班工具栏
    QWidget *doctorToolbar = new QWidget;
    QHBoxLayout *doctorToolbarLayout = new QHBoxLayout(doctorToolbar);
    doctorToolbarLayout->setContentsMargins(0, 0, 0, 0);
    
    refreshDoctorsBtn = new QPushButton("刷新医生排班");
    refreshDoctorsBtn->setObjectName("primaryBtn");
    registerBtn = new QPushButton("立即挂号");
    registerBtn->setObjectName("primaryBtn");
    
    doctorIdEdit = new QLineEdit;
    doctorIdEdit->setPlaceholderText("序号");
    doctorIdEdit->setFixedWidth(80);
    
    doctorNameEdit = new QLineEdit;
    doctorNameEdit->setPlaceholderText("医生姓名");
    doctorNameEdit->setFixedWidth(150);
    
    patientNameEdit = new QLineEdit;
    patientNameEdit->setPlaceholderText("患者账号");
    patientNameEdit->setEnabled(false);
    patientNameEdit->setText(m_patientName);
    patientNameEdit->setFixedWidth(150);

    doctorToolbarLayout->addWidget(refreshDoctorsBtn);
    doctorToolbarLayout->addSpacing(20);
    doctorToolbarLayout->addWidget(new QLabel("医生:"));
    doctorToolbarLayout->addWidget(doctorNameEdit);
    doctorToolbarLayout->addWidget(new QLabel("序号:"));
    doctorToolbarLayout->addWidget(doctorIdEdit);
    doctorToolbarLayout->addWidget(new QLabel("患者:"));
    doctorToolbarLayout->addWidget(patientNameEdit);
    doctorToolbarLayout->addSpacing(20);
    doctorToolbarLayout->addWidget(registerBtn);
    doctorToolbarLayout->addStretch();

    QLabel *doctorLabel = new QLabel("医生排班信息");
    doctorLabel->setObjectName("cardTitle");
    
    doctorTable = new QTableWidget;
    doctorTable->setColumnCount(8);
    doctorTable->setHorizontalHeaderLabels({"序号","账号","姓名","科室","工作时间","费用","已/上限","剩余"});
    doctorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    doctorCardLayout->addWidget(doctorLabel);
    doctorCardLayout->addWidget(doctorToolbar);
    doctorCardLayout->addWidget(doctorTable);

    // 我的预约卡片
    QWidget *appointmentCard = new QWidget;
    appointmentCard->setObjectName("appointmentCard");
    QVBoxLayout *appointmentCardLayout = new QVBoxLayout(appointmentCard);
    appointmentCardLayout->setContentsMargins(20, 20, 20, 20);
    appointmentCardLayout->setSpacing(16);

    // 预约工具栏
    QWidget *appointmentToolbar = new QWidget;
    QHBoxLayout *appointmentToolbarLayout = new QHBoxLayout(appointmentToolbar);
    appointmentToolbarLayout->setContentsMargins(0, 0, 0, 0);
    
    refreshAppointmentsBtn = new QPushButton("刷新我的预约");
    refreshAppointmentsBtn->setObjectName("primaryBtn");
    
    appointmentToolbarLayout->addWidget(refreshAppointmentsBtn);
    appointmentToolbarLayout->addStretch();

    QLabel *appointmentLabel = new QLabel("我的预约列表");
    appointmentLabel->setObjectName("cardTitle");
    
    appointmentsTable = new QTableWidget;
    appointmentsTable->setColumnCount(8);
    appointmentsTable->setHorizontalHeaderLabels({"ID","医生账号","医生姓名","日期","时间","科室","状态","费用"});
    appointmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    appointmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    appointmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    appointmentCardLayout->addWidget(appointmentLabel);
    appointmentCardLayout->addWidget(appointmentToolbar);
    appointmentCardLayout->addWidget(appointmentsTable);

    contentLayout->addWidget(doctorCard);
    contentLayout->addWidget(appointmentCard);
    layout->addWidget(contentArea);

    connect(refreshDoctorsBtn,&QPushButton::clicked,this,&AppointmentPage::requestDoctorSchedule);
    connect(refreshAppointmentsBtn,&QPushButton::clicked,this,&AppointmentPage::requestAppointments);
    connect(registerBtn,&QPushButton::clicked,this,&AppointmentPage::sendRegisterRequest);
    connect(doctorTable,&QTableWidget::cellClicked,this,[&](int r,int){ doctorIdEdit->setText(doctorTable->item(r,0)->text()); doctorNameEdit->setText(doctorTable->item(r,2)->text().isEmpty()? doctorTable->item(r,1)->text(): doctorTable->item(r,2)->text()); });

    // 服务化
    m_service = new PatientAppointmentService(m_client, this);
    connect(m_service, &PatientAppointmentService::doctorsFetched, this, [this](const QJsonArray& arr){
        doctorTable->setRowCount(arr.size()); int row=0;
        for (const auto &v : arr) {
            QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ doctorTable->setItem(row,c,new QTableWidgetItem(t)); };
            // 兼容不同医生列表响应的字段
            const bool hasSchedule = o.contains("doctorId") || o.contains("working_days");
            if (hasSchedule) {
                set(0, QString::number(o.value("doctorId").toInt(row+1)));
                set(1, o.value("doctor_username").toString(o.value("username").toString()));
                set(2, o.value("name").toString());
                set(3, o.value("department").toString());
                set(4, o.value("workTime").toString(o.value("working_days").toString()));
                set(5, QString::number(o.value("fee").toDouble(o.value("consultation_fee").toDouble()), 'f', 2));
                // 使用准确的今日预约数量
                int reserved = o.value("reservedPatients").toInt(o.value("today_appointments").toInt());
                int maxp = o.value("maxPatientsPerDay").toInt(o.value("today_max_appointments").toInt(o.value("max_patients_per_day").toInt()));
                int remain = o.value("remainingSlots").toInt(o.value("available_slots_today").toInt(maxp - reserved));
                set(6, QString("%1/%2").arg(reserved).arg(maxp));
                set(7, QString::number(remain));
            } else {
                set(0, QString::number(row+1));
                set(1, o.value("username").toString());
                set(2, o.value("name").toString());
                set(3, o.value("department").toString());
                set(4, QString("%1 (%2)").arg(o.value("title").toString()).arg(o.value("specialization").toString()));
                set(5, QString::number(o.value("consultation_fee").toDouble(),'f',2));
                // 使用准确的预约数量和上限
                int todayAppointments = o.value("today_appointments").toInt();
                int todayMaxAppointments = o.value("today_max_appointments").toInt();
                
                // 如果没有排班数据，使用默认值
                if (todayMaxAppointments == 0) {
                    todayMaxAppointments = o.value("max_patients_per_day").toInt(20);
                }
                
                set(6, QString("%1/%2").arg(todayAppointments).arg(todayMaxAppointments));
                set(7, QString::number(todayMaxAppointments - todayAppointments));
                
                qDebug() << "[ AppointmentPage ] 医生" << o.value("name").toString() 
                         << "today_appointments=" << todayAppointments 
                         << "today_max_appointments=" << todayMaxAppointments;
            }
            ++row;
        }
    });
    connect(m_service, &PatientAppointmentService::appointmentsFetched, this, [this](const QJsonArray& arr){
        appointmentsTable->setRowCount(arr.size()); int r=0;
        for(const auto &v: arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ appointmentsTable->setItem(r,c,new QTableWidgetItem(t)); };
            set(0,QString::number(o.value("id").toInt())); 
            set(1,o.value("doctor_username").toString()); 
            set(2,o.value("doctor_name").toString()); 
            set(3,o.value("appointment_date").toString()); 
            set(4,o.value("appointment_time").toString()); 
            set(5,o.value("department").toString()); 
            set(6,o.value("status").toString()); 
            set(7,QString::number(o.value("fee").toDouble(),'f',2)); 
            if(!o.value("doctor_title").toString().isEmpty() || !o.value("doctor_specialization").toString().isEmpty()) {
                QString tooltip = QString("医生职称: %1\n专业: %2")
                    .arg(o.value("doctor_title").toString())
                    .arg(o.value("doctor_specialization").toString());
                if (auto *it = appointmentsTable->item(r,2)) it->setToolTip(tooltip);
            }
            ++r; 
        }
    });
    connect(m_service, &PatientAppointmentService::createSucceeded, this, [this](const QString& msg){
        QMessageBox::information(this, "成功", msg.isEmpty()? QStringLiteral("挂号成功！"): msg);
        
        // 本地更新医生预约计数 - 找到刚刚预约的医生行并+1
        QString doctorUsername = "";
        QString selectedDoctorId = doctorIdEdit->text().trimmed();
        if (!selectedDoctorId.isEmpty()) {
            bool idOk = false;
            int idVal = selectedDoctorId.toInt(&idOk);
            if (idOk && idVal > 0 && idVal <= doctorTable->rowCount()) {
                int targetRow = idVal - 1; // 序号从1开始
                updateDoctorAppointmentCount(targetRow, 1); // +1
                doctorUsername = doctorTable->item(targetRow, 1) ? doctorTable->item(targetRow, 1)->text() : "";
            }
        }
        
        doctorNameEdit->clear(); doctorIdEdit->clear(); patientNameEdit->clear();
        requestDoctorSchedule(); requestAppointments(); // 依然进行完整刷新以确保数据准确性
    });
    connect(m_service, &PatientAppointmentService::createFailed, this, [this](const QString& err){
        QMessageBox::warning(this, "挂号失败", err.isEmpty()? QStringLiteral("挂号失败，请稍后重试"): err);
    });
    
    // 监听预约数量变化
    connect(m_service, &PatientAppointmentService::appointmentCountChanged, this, [this](const QString& doctorUsername, int delta){
        updateDoctorCountByUsername(doctorUsername, delta);
    });

    QTimer::singleShot(300,this,&AppointmentPage::requestDoctorSchedule);
    QTimer::singleShot(600,this,&AppointmentPage::requestAppointments);
}

void AppointmentPage::handleResponse(const QJsonObject &obj){
    QString type=obj.value("type").toString();
    qDebug() << "[ AppointmentPage ] 收到响应，类型:" << type;
    qDebug() << "[ AppointmentPage ] 完整响应:" << obj;
    
    if(type=="doctor_schedule_response"){
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); doctorTable->setRowCount(arr.size()); int row=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ doctorTable->setItem(row,c,new QTableWidgetItem(t)); };
            set(0,QString::number(o.value("doctorId").toInt())); set(1,o.value("doctor_username").toString()); set(2,o.value("name").toString()); set(3,o.value("department").toString()); set(4,o.value("workTime").toString()); set(5,QString::number(o.value("fee").toDouble(),'f',2)); int reserved=o.value("reservedPatients").toInt(); int maxp=o.value("maxPatientsPerDay").toInt(); int remain=o.value("remainingSlots").toInt(); set(6,QString("%1/%2").arg(reserved).arg(maxp)); set(7,QString::number(remain)); ++row; }
    } else if(type=="doctors_response"){
        // 处理来自get_all_doctors的响应，现在包含预约统计信息
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); doctorTable->setRowCount(arr.size()); int row=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ doctorTable->setItem(row,c,new QTableWidgetItem(t)); };
            set(0,QString::number(row+1)); // 序号从1开始
            set(1,o.value("username").toString()); // 医生账号
            set(2,o.value("name").toString()); // 医生姓名
            set(3,o.value("department").toString()); // 科室
            
            // 工作时间信息：优先使用working_days，否则使用职称信息
            QString workInfo = o.value("working_days").toString();
            if (workInfo.isEmpty()) {
                workInfo = QString("%1 (%2)").arg(o.value("title").toString()).arg(o.value("specialization").toString());
            }
            set(4, workInfo);
            
            set(5,QString::number(o.value("consultation_fee").toDouble(),'f',2)); // 挂号费
            
            // 预约统计：优先使用today_appointments和today_max_appointments
            int todayAppts = o.value("today_appointments").toInt(); 
            int todayMax = o.value("today_max_appointments").toInt();
            if (todayMax == 0) {
                // 如果没有排班数据，使用max_patients_per_day作为备用
                todayMax = o.value("max_patients_per_day").toInt();
            }
            set(6,QString("%1/%2").arg(todayAppts).arg(todayMax)); // 已预约/上限
            set(7,QString::number(todayMax - todayAppts)); // 剩余
            
            // 输出调试信息
            qDebug() << QString("[ AppointmentPage ] 医生 \"%1\" today_appointments= %2 today_max_appointments= %3")
                        .arg(o.value("name").toString()).arg(todayAppts).arg(todayMax);
            ++row; 
        }
    } else if(type=="doctors_schedule_overview_response"){
        // 保留之前的增强处理逻辑（备用）
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); doctorTable->setRowCount(arr.size()); int row=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ doctorTable->setItem(row,c,new QTableWidgetItem(t)); };
            set(0,QString::number(row+1)); // 临时序号
            set(1,o.value("username").toString()); 
            set(2,o.value("name").toString()); 
            set(3,o.value("department").toString()); 
            set(4,o.value("working_days").toString()); // 显示工作日信息
            set(5,QString::number(o.value("consultation_fee").toDouble(),'f',2)); 
            int todayAppts=o.value("today_appointments").toInt(); 
            int maxDaily=o.value("max_patients_per_day").toInt(); 
            int available=o.value("available_slots_today").toInt(); 
            set(6,QString("%1/%2").arg(todayAppts).arg(maxDaily)); 
            set(7,QString::number(available)); 
            ++row; 
        }
    } else if(type=="register_doctor_response" || type=="create_appointment_response"){
        static QString lastProcessedUuid;
        QString currentUuid = obj.value("request_uuid").toString();
        
        // 防止重复处理同一个响应
        if (!currentUuid.isEmpty() && currentUuid == lastProcessedUuid) {
            return;
        }
        lastProcessedUuid = currentUuid;
        
        if(obj.value("success").toBool()){
            QMessageBox::information(this,"成功","挂号成功！");
            // 清空输入框
            doctorNameEdit->clear();
            doctorIdEdit->clear();
            patientNameEdit->clear();
            // 刷新数据
            requestDoctorSchedule(); 
            requestAppointments();
        } else {
            QString errorMsg = obj.value("error").toString();
            if(errorMsg.isEmpty()) {
                errorMsg = "挂号失败，请稍后重试";
            }
            QMessageBox::warning(this,"挂号失败", errorMsg);
        }
    } else if(type=="appointments_response"){
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); appointmentsTable->setRowCount(arr.size()); int r=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ appointmentsTable->setItem(r,c,new QTableWidgetItem(t)); };
            set(0,QString::number(o.value("id").toInt())); 
            set(1,o.value("doctor_username").toString()); 
            set(2,o.value("doctor_name").toString()); 
            set(3,o.value("appointment_date").toString()); 
            set(4,o.value("appointment_time").toString()); 
            set(5,o.value("department").toString()); 
            set(6,o.value("status").toString()); 
            set(7,QString::number(o.value("fee").toDouble(),'f',2)); 
            
            // 设置工具提示显示更多医生信息
            if(!o.value("doctor_title").toString().isEmpty() || !o.value("doctor_specialization").toString().isEmpty()) {
                QString tooltip = QString("医生职称: %1\n专业: %2")
                    .arg(o.value("doctor_title").toString())
                    .arg(o.value("doctor_specialization").toString());
                appointmentsTable->item(r, 2)->setToolTip(tooltip); // 医生姓名列
            }
            ++r; 
        }
    }
}

void AppointmentPage::requestDoctorSchedule(){ m_service->fetchAllDoctors(); }

void AppointmentPage::requestAppointments(){ m_service->fetchAppointmentsForPatient(m_patientName); }

void AppointmentPage::sendRegisterRequest(){
    QString docName = doctorNameEdit->text().trimmed();
    QString docId   = doctorIdEdit->text().trimmed();
    if(docName.isEmpty()){ QMessageBox::warning(this,"提示","请输入或选择医生姓名"); return; }
    // 强制使用当前登录患者名称，避免手工输入造成用户不存在
    QString patient = m_patientName;
    if(patientNameEdit->text() != m_patientName)
        patientNameEdit->setText(m_patientName);

    // 尝试定位选中的医生行
    int targetRow = -1;
    bool idOk=false; int idVal = docId.toInt(&idOk);
    if(idOk && idVal>0 && idVal<=doctorTable->rowCount()){
        targetRow = idVal-1; // 序号列从1开始
    } else {
        // 通过姓名或账号匹配
        for(int r=0;r<doctorTable->rowCount();++r){
            QString nameCell = doctorTable->item(r,2)?doctorTable->item(r,2)->text():QString();
            QString userCell = doctorTable->item(r,1)?doctorTable->item(r,1)->text():QString();
            if(nameCell == docName || userCell == docName){ targetRow = r; break; }
        }
    }
    if(targetRow<0){ QMessageBox::warning(this,"提示","未找到对应医生，请刷新后重试"); return; }

    // 提取医生字段
    auto safeText=[&](int col){ auto *it=doctorTable->item(targetRow,col); return it?it->text():QString(); };
    QString doctorUsername = safeText(1);
    QString doctorRealName = safeText(2).isEmpty()? doctorUsername : safeText(2);
    QString department     = safeText(3);
    double fee             = safeText(5).toDouble();
    
    // 检查预约上限 - 在发送请求前进行客户端验证
    QString appointmentCountText = safeText(6); // "已预约/上限"
    QStringList countParts = appointmentCountText.split('/');
    if (countParts.size() == 2) {
        bool ok1, ok2;
        int currentCount = countParts[0].toInt(&ok1);
        int maxCount = countParts[1].toInt(&ok2);
        if (ok1 && ok2 && currentCount >= maxCount) {
            QMessageBox::warning(this, "预约失败", "当前医生预约数量已达上限，请等待或选择其他医生挂号");
            return;
        }
    }

    // 组装 create_appointment 请求（后端已有稳定接口）
    QJsonObject data;
    data["patient_username"] = patient;
    data["doctor_username"]  = doctorUsername;
    data["appointment_date"] = QDate::currentDate().toString("yyyy-MM-dd");
    data["appointment_time"] = QTime::currentTime().toString("HH:mm");
    data["department"]       = department;
    data["chief_complaint"]  = QString("预约挂号 - %1").arg(doctorRealName);
    data["fee"]              = fee;

    static int counter=1; const QString uuid = QString("appt_req_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(counter++);
    qDebug() << "[ AppointmentPage ] 发送 create_appointment 请求数据:" << data;
    m_service->createAppointment(data, uuid);
}

void AppointmentPage::updateDoctorAppointmentCount(int row, int delta) {
    if (row < 0 || row >= doctorTable->rowCount()) return;
    
    // 获取当前的"已预约/上限"列（第6列）
    auto* countItem = doctorTable->item(row, 6);
    if (!countItem) return;
    
    QString countText = countItem->text();
    QStringList parts = countText.split('/');
    if (parts.size() != 2) return;
    
    bool ok1, ok2;
    int currentCount = parts[0].toInt(&ok1);
    int maxCount = parts[1].toInt(&ok2);
    
    if (ok1 && ok2) {
        int newCount = qMax(0, currentCount + delta); // 确保不为负数
        newCount = qMin(newCount, maxCount); // 确保不超过上限
        countItem->setText(QString("%1/%2").arg(newCount).arg(maxCount));
        
        // 同时更新"剩余"列（第7列）
        auto* remainItem = doctorTable->item(row, 7);
        if (remainItem) {
            remainItem->setText(QString::number(maxCount - newCount));
        }
        
        qDebug() << "[ AppointmentPage ] 更新医生预约计数: " << countText << " -> " << countItem->text();
    }
}

void AppointmentPage::updateDoctorCountByUsername(const QString& doctorUsername, int delta) {
    if (doctorUsername.isEmpty() || !doctorTable) return;
    
    // 查找对应医生的行
    for (int row = 0; row < doctorTable->rowCount(); ++row) {
        auto* usernameItem = doctorTable->item(row, 1); // 账号列
        if (usernameItem && usernameItem->text() == doctorUsername) {
            updateDoctorAppointmentCount(row, delta);
            break;
        }
    }
}
