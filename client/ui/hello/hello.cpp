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
    doctorInfoWidget = nullptr;
    patientInfoWidget = nullptr;

    stackedWidget->addWidget(loginWidget);

    connect(loginWidget, &LoginWidget::doctorLoggedIn, this, &Hello::showDoctorUI);
    connect(loginWidget, &LoginWidget::patientLoggedIn, this, &Hello::showPatientUI);
    connect(doctorInfoWidget, &DoctorInfoWidget::backToLogin, this, &Hello::showLoginUI);
    connect(patientInfoWidget, &PatientInfoWidget::backToLogin, this, &Hello::showLoginUI);

    stackedWidget->setCurrentWidget(loginWidget);
}

Hello::~Hello() {}

void Hello::showDoctorUI(const QString &doctorName)
{
    if (!doctorInfoWidget) {
        doctorInfoWidget = new DoctorInfoWidget(doctorName, this);
        stackedWidget->addWidget(doctorInfoWidget);
        connect(doctorInfoWidget, &DoctorInfoWidget::backToLogin, this, &Hello::showLoginUI);
    }
    stackedWidget->setCurrentWidget(doctorInfoWidget);
}

void Hello::showPatientUI(const QString &patientName)
{
    if (!patientInfoWidget) {
        patientInfoWidget = new PatientInfoWidget(patientName, this);
        stackedWidget->addWidget(patientInfoWidget);
        connect(patientInfoWidget, &PatientInfoWidget::backToLogin, this, &Hello::showLoginUI);
    }
    stackedWidget->setCurrentWidget(patientInfoWidget);
}

void Hello::showLoginUI()
{
    stackedWidget->setCurrentWidget(loginWidget);
}
