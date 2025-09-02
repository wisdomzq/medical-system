#pragma once
#include "basepage.h"
#include <QJsonObject>
class QTableWidget; class QPushButton; class QLineEdit;
class HospitalizationService;

class HospitalPage : public BasePage {
    Q_OBJECT
public:
    explicit HospitalPage(CommunicationClient *c,const QString &patient,QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj); // 将废弃
signals:
    void requestDetail(int id); // 预留
private slots:
    void refresh();
    void searchByDoctor();
private:
    QTableWidget *table=nullptr; QPushButton *refreshBtn=nullptr; QLineEdit *filterDoctorEdit=nullptr; 
    QPushButton *searchBtn=nullptr;
    HospitalizationService* m_service = nullptr; // 非拥有
};
