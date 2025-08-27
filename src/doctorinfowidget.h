#ifndef DOCTORINFOWIDGET_H
#define DOCTORINFOWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include "dbmanager.h"

class QTabWidget;

class DoctorInfoWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoWidget(QWidget *parent = nullptr);
    ~DoctorInfoWidget();
    void setDoctorName(const QString &doctorName);

private:
    QTabWidget *tabWidget;
    QString currentDoctorName;
    QLineEdit *nameEdit;
    QLineEdit *departmentEdit;
    QLineEdit *phoneEdit;

    QWidget *createAppointmentPage();
    QWidget *createCasePage();
    QWidget *createCommunicationPage();
    QWidget *createProfilePage();
    void loadProfile();

private slots:
    void updateProfile();

signals:
    void backToLogin();
};

#endif // DOCTORINFOWIDGET_H
