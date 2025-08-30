#ifndef DOCTORLISTPAGE_H
#define DOCTORLISTPAGE_H

#include "basepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>

class DoctorInfoPage;

class DoctorListPage : public BasePage
{
    Q_OBJECT

public:
    explicit DoctorListPage(CommunicationClient* client, const QString& patientName, QWidget *parent = nullptr);

private slots:
    void onSearchClicked();
    void onViewDoctorClicked();
    void onDoctorTableDoubleClicked(int row, int column);
    void onDoctorInfoClosed();

private:
    void setupUI();
    void loadDoctorList();
    void displayDoctors(const QJsonArray& doctors);
    void openDoctorInfo(const QString& doctorUsername);
    
    // UI 组件
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchLayout;
    QLineEdit* m_searchEdit;
    QComboBox* m_departmentCombo;
    QPushButton* m_searchButton;
    QTableWidget* m_doctorTable;
    QPushButton* m_viewButton;
    
    // 数据
    QJsonArray m_doctors;
    
    // 医生详情页面
    DoctorInfoPage* m_doctorInfoPage;
};

#endif // DOCTORLISTPAGE_H
