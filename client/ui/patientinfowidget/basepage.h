// 基础页面类，提供 CommunicationClient 指针与患者用户名
#pragma once
#include <QWidget>
#include <QString>

class CommunicationClient;

class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(CommunicationClient *client, const QString &patientName, QWidget *parent=nullptr)
        : QWidget(parent), m_client(client), m_patientName(patientName) {}
    virtual ~BasePage() = default;

protected:
    CommunicationClient *m_client = nullptr;
    QString m_patientName;
};
