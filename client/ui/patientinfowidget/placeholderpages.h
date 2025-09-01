#pragma once
#include "basepage.h"
// 保留健康评估占位；医生信息已在 doctorinfopage.h 定义，避免重名冲突
class HealthAssessmentPage : public BasePage { public: HealthAssessmentPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
