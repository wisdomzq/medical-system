#pragma once
#include "basepage.h"
#include <QJsonObject>

class QLineEdit; class QPushButton;

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
    QLineEdit *nameEdit=nullptr,*ageEdit=nullptr,*phoneEdit=nullptr,*addressEdit=nullptr;
};
