#include "loginwidget.h"
#include "doctorinfowidget.h"
#include "patientinfowidget.h"
#include <QStackedWidget>
#pragma once
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
class CommunicationClient;

class Hello : public QMainWindow {
    Q_OBJECT
public:
    Hello(QWidget* parent = nullptr);
    ~Hello();
private:
    QStackedWidget *stackedWidget;
    LoginWidget *loginWidget;
    DoctorInfoWidget *doctorInfoWidget;
    PatientInfoWidget *patientInfoWidget;
    CommunicationClient *sharedClient {nullptr};

private slots:
    void showDoctorUI(const QString &doctorName);
    void showPatientUI(const QString &patientName);
    void showLoginUI();
};