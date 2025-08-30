#ifndef ADVICEPAGE_H
#define ADVICEPAGE_H

#include "basepage.h"
#include <QJsonObject>
#include <QJsonArray>

class QTableWidget;
class QPushButton;
class QLabel;

class AdvicePage : public BasePage {
    Q_OBJECT

public:
    explicit AdvicePage(CommunicationClient *client, const QString &patientName, QWidget *parent = nullptr);

private slots:
    void refreshList();
    void onDetailsClicked();
    void onTableDoubleClick(int row, int column);
    void onPrescriptionClicked();

private:
    void setupUI();
    void populateTable(const QJsonArray &advices);
    void showAdviceDetails(const QJsonObject &details);
    void requestAdviceList();
    void requestAdviceDetails(int adviceId);
    void sendJson(const QJsonObject &obj);
    void handleResponse(const QJsonObject &obj);

    QTableWidget *m_table;
    QPushButton *m_refreshBtn;
    QPushButton *m_detailsBtn;
    QPushButton *m_prescriptionBtn;
    QLabel *m_countLabel;
    QLabel *m_statusLabel;

    QJsonArray m_advices;
    int m_selectedRow = -1;

signals:
    void prescriptionRequested(int prescriptionId);
};

#endif // ADVICEPAGE_H
