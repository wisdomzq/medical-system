#ifndef PATIENTINFOWIDGET_H
#define PATIENTINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "core/network/src/client/communicationclient.h"

class QTabWidget;

class PatientInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PatientInfoWidget(const QString &patientName, QWidget *parent = nullptr);
    ~PatientInfoWidget();

private:
    QTabWidget *tabWidget;
    QString m_patientName;
    QLineEdit *nameEdit;
    QLineEdit *ageEdit;
    QLineEdit *phoneEdit;
    QLineEdit *addressEdit;
    CommunicationClient *m_communicationClient;

    QWidget *createAppointmentPage();
    QWidget *createCasePage();
    QWidget *createCommunicationPage();
    QWidget *createProfilePage();
    void requestPatientInfo();

private slots:
    void updateProfile();
    void onResponseReceived(const QJsonObject &response);

signals:
    void backToLogin();
};

#endif // PATIENTINFOWIDGET_H
