#include "profilepage.h"
#include "core/network/src/client/communicationclient.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QMessageBox>

ProfilePage::ProfilePage(CommunicationClient *c,const QString &patient,QWidget *parent)
    : BasePage(c,patient,parent)
{
    QVBoxLayout *layout=new QVBoxLayout(this);
    QFormLayout *form=new QFormLayout; nameEdit=new QLineEdit; ageEdit=new QLineEdit; phoneEdit=new QLineEdit; addressEdit=new QLineEdit;
    form->addRow("姓名:",nameEdit); form->addRow("年龄:",ageEdit); form->addRow("电话:",phoneEdit); form->addRow("地址:",addressEdit);
    QPushButton *updateBtn=new QPushButton("更新信息"); QPushButton *backBtn=new QPushButton("返回登录");
    connect(updateBtn,&QPushButton::clicked,this,&ProfilePage::updateProfile);
    connect(backBtn,&QPushButton::clicked,this,[this]{ emit backToLogin(); });
    layout->addLayout(form); layout->addWidget(updateBtn); layout->addWidget(backBtn); layout->addStretch();
    QTimer::singleShot(200,this,&ProfilePage::requestPatientInfo);
}

void ProfilePage::handleResponse(const QJsonObject &obj){
    if(obj.value("type").toString()=="patient_info_response" && obj.value("success").toBool()){
        QJsonObject d=obj.value("data").toObject();
        nameEdit->setText(d.value("name").toString());
        ageEdit->setText(QString::number(d.value("age").toInt()));
        phoneEdit->setText(d.value("phone").toString());
        addressEdit->setText(d.value("address").toString());
    }
}

void ProfilePage::requestPatientInfo(){ QJsonObject req; req["action"]="get_patient_info"; req["username"]=m_patientName; m_client->sendJson(req);} 
void ProfilePage::updateProfile(){ QMessageBox::information(this,"提示","更新功能需要服务器端实现。"); }
