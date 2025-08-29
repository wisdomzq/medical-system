#ifndef DBTERMINAL_H
#define DBTERMINAL_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include "database.h"

class DBTerminal : public QObject
{
    Q_OBJECT
    
public:
    explicit DBTerminal(DBManager* dbManager, QObject *parent = nullptr);
    
    // 显示各类数据
    void showDatabaseOverview();    // 添加这行
    void showUsers();
    void showDoctors();
    void showPatients();
    void showAppointments();
    void showMedicalRecords();
    void showMedicalAdvices();
    void showPrescriptions();
    void showMedications();
    void showStatistics();
    
    // 数据监控
    void startMonitoring(int intervalMs = 5000);
    void stopMonitoring();

private slots:
    void updateDisplay();

private:
    DBManager* m_dbManager;
    QTimer* m_displayTimer;
    
    void printTableHeader(const QStringList& headers);
    void printTableRow(const QStringList& data);
    void printSeparator(int width = 80);
};

#endif // DBTERMINAL_H