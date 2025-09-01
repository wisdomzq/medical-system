#include "hello.h"
#include <QFont>
#include <QSpacerItem>
#include "core/network/communicationclient.h"
#include "core/network/protocol.h"

Hello::Hello(QWidget* parent)
    : QMainWindow(parent)
{
    resize(900, 600);

    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    // 创建全局共享通信客户端，仅此一处 new
    sharedClient = new CommunicationClient(this);
    sharedClient->connectToServer("127.0.0.1", Protocol::SERVER_PORT);

    loginWidget = new LoginWidget(sharedClient, this);
    doctorInfoWidget = nullptr;
    patientInfoWidget = nullptr;

    stackedWidget->addWidget(loginWidget);

    connect(loginWidget, &LoginWidget::doctorLoggedIn, this, &Hello::showDoctorUI);
    connect(loginWidget, &LoginWidget::patientLoggedIn, this, &Hello::showPatientUI);

    stackedWidget->setCurrentWidget(loginWidget);
}

Hello::~Hello() {}

void Hello::showDoctorUI(const QString &doctorName)
{
    if (!doctorInfoWidget) {
        doctorInfoWidget = new DoctorInfoWidget(doctorName, sharedClient, this);
        stackedWidget->addWidget(doctorInfoWidget);
        connect(doctorInfoWidget, &DoctorInfoWidget::backToLogin, this, &Hello::showLoginUI);
    }
    stackedWidget->setCurrentWidget(doctorInfoWidget);
}

void Hello::showPatientUI(const QString &patientName)
{
    if (!patientInfoWidget) {
        patientInfoWidget = new PatientInfoWidget(patientName, sharedClient, this);
        stackedWidget->addWidget(patientInfoWidget);
        connect(patientInfoWidget, &PatientInfoWidget::backToLogin, this, &Hello::showLoginUI);
    }
    stackedWidget->setCurrentWidget(patientInfoWidget);
}

void Hello::showLoginUI()
{
    stackedWidget->setCurrentWidget(loginWidget);
}
