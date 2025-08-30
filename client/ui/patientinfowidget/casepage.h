#ifndef CASEPAGE_H
#define CASEPAGE_H

#include "basepage.h"
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>

class CasePage : public BasePage
{
    Q_OBJECT

public:
    explicit CasePage(CommunicationClient *client, const QString &patientName, QWidget *parent = nullptr);

protected slots:
    void onConnected();
    void onMessageReceived(const QJsonObject &message);

private slots:
    void onBackButtonClicked();
    void onRowDoubleClicked(int row, int column);
    void loadMedicalRecords();

private:
    void setupUI();
    void populateTable(const QJsonArray &records);
    
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_backButton;
    QTableWidget *m_recordTable;
    
    QJsonArray m_records; // 存储当前的病例记录

signals:
    void backRequested(); // 返回按钮点击信号
};

#endif // CASEPAGE_H
