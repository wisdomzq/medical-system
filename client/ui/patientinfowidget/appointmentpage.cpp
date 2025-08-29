#include "appointmentpage.h"
#include "core/network/src/client/communicationclient.h"
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

AppointmentPage::AppointmentPage(CommunicationClient *c, const QString &patient, QWidget *parent)
    : BasePage(c, patient, parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *toolbar = new QHBoxLayout; refreshDoctorsBtn=new QPushButton("刷新医生排班"); registerBtn=new QPushButton("挂号");
    doctorIdEdit=new QLineEdit; doctorIdEdit->setPlaceholderText("(序号)"); doctorIdEdit->setFixedWidth(70);
    doctorNameEdit=new QLineEdit; doctorNameEdit->setPlaceholderText("医生姓名"); doctorNameEdit->setFixedWidth(120);
    patientNameEdit=new QLineEdit; patientNameEdit->setPlaceholderText("患者(默认本人)");
    toolbar->addWidget(refreshDoctorsBtn);
    toolbar->addWidget(new QLabel("医生:")); toolbar->addWidget(doctorNameEdit);
    toolbar->addWidget(new QLabel("序号:")); toolbar->addWidget(doctorIdEdit);
    toolbar->addWidget(new QLabel("患者:")); toolbar->addWidget(patientNameEdit);
    toolbar->addWidget(registerBtn); toolbar->addStretch();

    doctorTable=new QTableWidget; doctorTable->setColumnCount(8); doctorTable->setHorizontalHeaderLabels({"序号","账号","姓名","科室","工作时间","费用","已/上限","剩余"}); doctorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); doctorTable->setSelectionBehavior(QAbstractItemView::SelectRows); doctorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addLayout(toolbar); layout->addWidget(new QLabel("医生排班")); layout->addWidget(doctorTable);

    QHBoxLayout *apptBar=new QHBoxLayout; refreshAppointmentsBtn=new QPushButton("刷新我的预约"); apptBar->addWidget(refreshAppointmentsBtn); apptBar->addStretch(); layout->addSpacing(12); layout->addLayout(apptBar);
    appointmentsTable=new QTableWidget; appointmentsTable->setColumnCount(8); appointmentsTable->setHorizontalHeaderLabels({"ID","医生账号","医生姓名","日期","时间","科室","状态","费用"}); appointmentsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); appointmentsTable->setSelectionBehavior(QAbstractItemView::SelectRows); appointmentsTable->setEditTriggers(QAbstractItemView::NoEditTriggers); layout->addWidget(new QLabel("我的预约列表")); layout->addWidget(appointmentsTable);

    connect(refreshDoctorsBtn,&QPushButton::clicked,this,&AppointmentPage::requestDoctorSchedule);
    connect(refreshAppointmentsBtn,&QPushButton::clicked,this,&AppointmentPage::requestAppointments);
    connect(registerBtn,&QPushButton::clicked,this,&AppointmentPage::sendRegisterRequest);
    connect(doctorTable,&QTableWidget::cellClicked,this,[&](int r,int){ doctorIdEdit->setText(doctorTable->item(r,0)->text()); doctorNameEdit->setText(doctorTable->item(r,2)->text().isEmpty()? doctorTable->item(r,1)->text(): doctorTable->item(r,2)->text()); });

    QTimer::singleShot(300,this,&AppointmentPage::requestDoctorSchedule);
    QTimer::singleShot(600,this,&AppointmentPage::requestAppointments);
}

void AppointmentPage::handleResponse(const QJsonObject &obj){
    QString type=obj.value("type").toString();
    if(type=="doctor_schedule_response"){
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); doctorTable->setRowCount(arr.size()); int row=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ doctorTable->setItem(row,c,new QTableWidgetItem(t)); };
            set(0,QString::number(o.value("doctorId").toInt())); set(1,o.value("doctor_username").toString()); set(2,o.value("name").toString()); set(3,o.value("department").toString()); set(4,o.value("workTime").toString()); set(5,QString::number(o.value("fee").toDouble(),'f',2)); int reserved=o.value("reservedPatients").toInt(); int maxp=o.value("maxPatientsPerDay").toInt(); int remain=o.value("remainingSlots").toInt(); set(6,QString("%1/%2").arg(reserved).arg(maxp)); set(7,QString::number(remain)); ++row; }
    } else if(type=="register_doctor_response"){
        if(obj.value("success").toBool()){
            QMessageBox::information(this,"成功","挂号成功");
            requestDoctorSchedule(); requestAppointments();
        } else {
            QMessageBox::warning(this,"失败",obj.value("error").toString());
        }
    } else if(type=="appointments_response"){
        if(!obj.value("success").toBool())return;
        QJsonArray arr=obj.value("data").toArray(); appointmentsTable->setRowCount(arr.size()); int r=0;
        for(auto v:arr){ QJsonObject o=v.toObject(); auto set=[&](int c,const QString&t){ appointmentsTable->setItem(r,c,new QTableWidgetItem(t)); };
            set(0,QString::number(o.value("id").toInt())); set(1,o.value("doctor_username").toString()); set(2,o.value("doctor_name").toString()); set(3,o.value("appointment_date").toString()); set(4,o.value("appointment_time").toString()); set(5,o.value("department").toString()); set(6,o.value("status").toString()); set(7,QString::number(o.value("fee").toDouble(),'f',2)); ++r; }
    }
}

void AppointmentPage::requestDoctorSchedule(){ QJsonObject req; req["action"]="get_doctor_schedule"; m_client->sendJson(req);} 
void AppointmentPage::requestAppointments(){ QJsonObject req; req["action"]="get_appointments_by_patient"; req["username"]=m_patientName; m_client->sendJson(req);} 
void AppointmentPage::sendRegisterRequest(){ QString docName=doctorNameEdit->text().trimmed(); if(docName.isEmpty()){ QMessageBox::warning(this,"提示","请输入或选择医生姓名"); return;} QString patient=patientNameEdit->text().trimmed(); if(patient.isEmpty()) patient=m_patientName; QJsonObject req; req["action"]="register_doctor"; req["doctor_name"]=docName; req["patientName"]=patient; m_client->sendJson(req);} 
