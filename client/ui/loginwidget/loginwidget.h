#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QStackedWidget>
#include <QFormLayout>
#include "core/network/src/client/communicationclient.h"

class LoginWidget : public QWidget {
    Q_OBJECT
public:
    explicit LoginWidget(QWidget* parent = nullptr);
    ~LoginWidget();

private:
    // The main widget that holds all pages
    QStackedWidget* stackedWidget;

    // --- Doctor Widgets ---
    // Login Page
    QLineEdit* doctorLoginNameEdit;
    QLineEdit* doctorLoginPasswordEdit;
    QPushButton* doctorLoginButton;
    QPushButton* goToDoctorRegisterButton;
    QPushButton* backFromDoctorLoginButton; // <-- 新增：从医生登录返回的按钮
    // Registration Page
    QLineEdit* doctorRegisterNameEdit;
    QLineEdit* doctorRegisterPasswordEdit;
    QLineEdit* doctorDepartmentEdit;
    QLineEdit* doctorPhoneEdit;
    QPushButton* doctorRegisterButton;
    QPushButton* backToDoctorLoginButton;

    // --- Patient Widgets ---
    // Login Page
    QLineEdit* patientLoginNameEdit;
    QLineEdit* patientLoginPasswordEdit;
    QPushButton* patientLoginButton;
    QPushButton* goToPatientRegisterButton;
    QPushButton* backFromPatientLoginButton; // <-- 新增：从病人登录返回的按钮
    // Registration Page
    QLineEdit* patientRegisterNameEdit;
    QLineEdit* patientRegisterPasswordEdit;
    QLineEdit* patientAgeEdit;
    QLineEdit* patientPhoneEdit;
    QLineEdit* patientAddressEdit;
    QPushButton* patientRegisterButton;
    QPushButton* backToPatientLoginButton;

    CommunicationClient* m_communicationClient;

private:
    // Helper function to create the main title label
    QLabel* createTitleLabel();

private slots:
    // --- Navigation Slots ---
    void showInitialSelection();
    void showDoctorLogin();
    void showPatientLogin();
    void showDoctorRegister();
    void showPatientRegister();

    // --- Action Slots ---
    void onDoctorLogin();
    void onPatientLogin();
    void onDoctorRegister();
    void onPatientRegister();
    void onResponseReceived(const QJsonObject &response);

signals:
    void doctorLoggedIn(const QString &doctorName);
    void patientLoggedIn(const QString &patientName);
};