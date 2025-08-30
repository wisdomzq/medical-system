#include "placeholderpages.h"
#include <QVBoxLayout>
#include <QLabel>

// 为每个占位页面添加简单标签，后续可扩展
template<typename T> static void ensureLayout(T *w,const char* text){ auto *lay=new QVBoxLayout(w); lay->addWidget(new QLabel(QString::fromUtf8(text))); lay->addStretch(); }

CommunicationPage::CommunicationPage(CommunicationClient *c,const QString&p,QWidget*parent):BasePage(c,p,parent){ ensureLayout(this,"医患沟通模块"); }
DoctorInfoPage::DoctorInfoPage(CommunicationClient *c,const QString&p,QWidget*parent):BasePage(c,p,parent){ ensureLayout(this,"查看医生信息"); }
HealthAssessmentPage::HealthAssessmentPage(CommunicationClient *c,const QString&p,QWidget*parent):BasePage(c,p,parent){ ensureLayout(this,"健康评估"); }
