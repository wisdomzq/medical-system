#pragma once
#include "basepage.h"
#include <QJsonObject>
#include <QJsonArray>

class QTableWidget;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QHBoxLayout;
class QTextEdit;
class QDialog;
class PrescriptionService;

class PrescriptionPage : public BasePage {
    Q_OBJECT
public:
    explicit PrescriptionPage(CommunicationClient *c, const QString &patient, QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj);

private slots:
    void refreshList();
    void onDetailsClicked();
    void onTableDoubleClick(int row, int column);

private:
    void setupUI();
    void populateTable(const QJsonArray &prescriptions);
    void showPrescriptionDetails(const QJsonObject &details);
    void requestPrescriptionList();
    void requestPrescriptionDetails(int prescriptionId);
    void sendJson(const QJsonObject &obj);
    
    QTableWidget *m_table;
    QPushButton *m_refreshBtn;
    QPushButton *m_detailsBtn;
    QLabel *m_statusLabel;
    QLabel *m_countLabel;
    
    QJsonArray m_prescriptions; // 缓存处方数据
    int m_selectedRow = -1;
    PrescriptionService* m_service = nullptr; // 非拥有
};
