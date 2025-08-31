#pragma once
#include "basepage.h"
#include <QJsonObject>

class PatientService;

class QLineEdit; 
class QPushButton;
class QSpinBox;
class QComboBox;

class ProfilePage : public BasePage {
    Q_OBJECT
public:
    explicit ProfilePage(CommunicationClient *c,const QString &patient,QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj);

signals:
    void backToLogin();

private slots:
    void requestPatientInfo();
    void updateProfile();

private:
    // 基本信息
    QLineEdit *nameEdit_ = nullptr;
    QSpinBox *ageEdit_ = nullptr;
    QComboBox *genderEdit_ = nullptr;
    QLineEdit *phoneEdit_ = nullptr;
    QLineEdit *emailEdit_ = nullptr;
    QLineEdit *addressEdit_ = nullptr;
    
    // 身份证和紧急联系人信息
    QLineEdit *idCardEdit_ = nullptr;
    QLineEdit *emergencyContactEdit_ = nullptr;
    QLineEdit *emergencyPhoneEdit_ = nullptr;
    
    // 按钮
    QPushButton *refreshBtn_ = nullptr;
    QPushButton *updateBtn_ = nullptr;

    // 服务
    PatientService* patientService_ = nullptr;
};
