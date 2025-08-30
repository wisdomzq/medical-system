#pragma once
#include "basepage.h"

class CommunicationPage : public BasePage { public: CommunicationPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class DoctorInfoPage : public BasePage { public: DoctorInfoPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class HealthAssessmentPage : public BasePage { public: HealthAssessmentPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
