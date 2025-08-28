#ifndef PATIENTINFOWIDGET_H
#define PATIENTINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "tcpclient.h"

class QTabWidget;

class PatientInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PatientInfoWidget(QWidget *parent = nullptr);
    ~PatientInfoWidget();
    void setPatientName(const QString &patientName);

private:
    QTabWidget *tabWidget;
    QString currentPatientName;
    QLineEdit *nameEdit;
    QLineEdit *ageEdit;
    QLineEdit *phoneEdit;
    QLineEdit *addressEdit;
    TcpClient *m_tcpClient;

    QWidget *createAppointmentPage();
    QWidget *createCasePage();
    QWidget *createCommunicationPage();
    QWidget *createProfilePage();
    void loadProfile();

private slots:
    void updateProfile();
    void onResponseReceived(const QJsonObject &response);

signals:
    void backToLogin();
};

#endif // PATIENTINFOWIDGET_H
