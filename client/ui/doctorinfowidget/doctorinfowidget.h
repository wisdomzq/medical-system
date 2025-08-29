#ifndef DOCTORINFOWIDGET_H
#define DOCTORINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "core/network/src/client/communicationclient.h"

class QTabWidget;

class DoctorInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoWidget(const QString &doctorName, QWidget *parent = nullptr);
    ~DoctorInfoWidget();

private:
    QTabWidget *tabWidget;
    QString m_doctorName;
    QLineEdit *nameEdit;
    QLineEdit *departmentEdit;
    QLineEdit *phoneEdit;
    CommunicationClient *m_communicationClient;

    QWidget *createAppointmentPage();
    QWidget *createCasePage();
    QWidget *createCommunicationPage();
    QWidget *createProfilePage();
    void requestDoctorInfo();

private slots:
    void updateProfile();
    void onResponseReceived(const QJsonObject &response);

signals:
    void backToLogin();
};

#endif // DOCTORINFOWIDGET_H
