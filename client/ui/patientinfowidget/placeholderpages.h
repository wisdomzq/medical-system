#pragma once
#include "basepage.h"

class CasePage : public BasePage { public: CasePage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class CommunicationPage : public BasePage { public: CommunicationPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class DoctorInfoPage : public BasePage { public: DoctorInfoPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class AdvicePage : public BasePage { public: AdvicePage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class PrescriptionPage : public BasePage { public: PrescriptionPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
class HealthAssessmentPage : public BasePage { public: HealthAssessmentPage(CommunicationClient *c,const QString&p,QWidget*parent=nullptr); };
