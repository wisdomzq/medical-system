#pragma once
#include "basepage.h"
#include <QJsonObject>
class QTableWidget; class QPushButton; class QLineEdit;

class HospitalPage : public BasePage {
    Q_OBJECT
public:
    explicit HospitalPage(CommunicationClient *c,const QString &patient,QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj);
signals:
    void requestDetail(int id); // 预留
private slots:
    void refresh();
private:
    QTableWidget *table=nullptr; QPushButton *refreshBtn=nullptr; QLineEdit *filterDoctorEdit=nullptr; };
