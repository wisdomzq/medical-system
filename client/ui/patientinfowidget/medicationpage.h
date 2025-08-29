#pragma once
#include "basepage.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QNetworkReply>
#include <QHash>

class MedicationSearchPage : public BasePage {
    Q_OBJECT
public:
    explicit MedicationSearchPage(CommunicationClient *c, const QString &patient, QWidget *parent=nullptr);
    void handleResponse(const QJsonObject &obj);
private slots:
    void onSearch();
    void onImageDownloaded();
private:
    QLineEdit *m_searchEdit;
    QPushButton *m_searchBtn;
    QPushButton *m_remoteBtn;
    QTableWidget *m_table;
    QHash<QNetworkReply*, int> m_replyRowMap; // reply -> row index
    void sendSearchRequest(const QString &keyword);
    void populateTable(const QJsonArray &arr);
    void fetchImageForRow(int row, const QString &medName);
    void remoteSearch();
};
