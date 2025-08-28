#include "hello.h"
#include <QFont>
#include <QSpacerItem>

Hello::Hello(QWidget* parent)
    : QMainWindow(parent)
{
    resize(900, 600);

    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    loginWidget = new LoginWidget(this);
    doctorInfoWidget = new DoctorInfoWidget(this);
    patientInfoWidget = new PatientInfoWidget(this);

    stackedWidget->addWidget(loginWidget);
    stackedWidget->addWidget(doctorInfoWidget);
    stackedWidget->addWidget(patientInfoWidget);

    connect(loginWidget, &LoginWidget::doctorLoggedIn, this, &Hello::showDoctorUI);
    connect(loginWidget, &LoginWidget::patientLoggedIn, this, &Hello::showPatientUI);
    connect(doctorInfoWidget, &DoctorInfoWidget::backToLogin, this, &Hello::showLoginUI);
    connect(patientInfoWidget, &PatientInfoWidget::backToLogin, this, &Hello::showLoginUI);

    stackedWidget->setCurrentWidget(loginWidget);
}

Hello::~Hello() {}

void Hello::showDoctorUI(const QString &doctorName)
{
    doctorInfoWidget->setDoctorName(doctorName);
    stackedWidget->setCurrentWidget(doctorInfoWidget);
}

void Hello::showPatientUI(const QString &patientName)
{
    patientInfoWidget->setPatientName(patientName);
    stackedWidget->setCurrentWidget(patientInfoWidget);
}

void Hello::showLoginUI()
{
    stackedWidget->setCurrentWidget(loginWidget);
}
