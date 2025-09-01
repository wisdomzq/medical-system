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
class MedicalRecordService;

class CasePage : public BasePage
{
    Q_OBJECT

public:
    explicit CasePage(CommunicationClient *client, const QString &patientName, QWidget *parent = nullptr);

protected slots:
    void onConnected();

private slots:
    void onRefreshButtonClicked();
    void onRowDoubleClicked(int row, int column);
    void loadMedicalRecords();

private:
    void setupUI();
    void populateTable(const QJsonArray &records);
    MedicalRecordService* m_service = nullptr; // 非拥有
    
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_backButton;
    QTableWidget *m_recordTable;
    
    QJsonArray m_records; // 存储当前的病例记录
};

#endif // CASEPAGE_H
