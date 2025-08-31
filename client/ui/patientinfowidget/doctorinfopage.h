#ifndef DOCTORINFOPAGE_H
#define DOCTORINFOPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include "core/services/doctorprofileservice.h"
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

class CommunicationClient;

class DoctorInfoPage : public QWidget
{
    Q_OBJECT

public:
    explicit DoctorInfoPage(const QString& doctorUsername, CommunicationClient* client, QWidget *parent = nullptr);

private slots:
    void onCloseButtonClicked();

private:
    void setupUI();
    void loadDoctorInfo();
    void displayDoctorInfo(const QJsonObject& doctorData);
    void displayBasicInfo(const QJsonObject& doctorInfo);
    void displaySchedule(const QJsonArray& schedules);
    void displayStatistics(const QJsonObject& statistics);
    
    QString m_doctorUsername;
    CommunicationClient* m_client;
    DoctorProfileService* m_service = nullptr; // 非拥有
    
    // UI 组件
    QVBoxLayout* m_mainLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
    QPushButton* m_closeButton;
    
    // 信息显示区域
    QFrame* m_basicInfoFrame;
    QFrame* m_scheduleFrame;
    QFrame* m_statisticsFrame;
    
    QLabel* m_photoLabel;
    QLabel* m_nameLabel;
    QLabel* m_workNumberLabel;
    QLabel* m_departmentLabel;
    QLabel* m_titleLabel;
    QLabel* m_specializationLabel;
    QLabel* m_consultationFeeLabel;
    QLabel* m_maxPatientsLabel;
    QLabel* m_experienceLabel;
    
    QGridLayout* m_scheduleLayout;
    QVBoxLayout* m_statisticsLayout;

signals:
    void closeRequested();
};

#endif // DOCTORINFOPAGE_H
