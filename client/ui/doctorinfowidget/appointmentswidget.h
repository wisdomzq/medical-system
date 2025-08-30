#ifndef APPOINTMENTSWIDGET_H
#define APPOINTMENTSWIDGET_H

#include <QWidget>
#include <QString>
#include <QJsonObject>

class QTableWidget;
class QPushButton;
class CommunicationClient;

class AppointmentsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentsWidget(const QString& doctorName, QWidget* parent=nullptr);
    explicit AppointmentsWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent=nullptr);
    
    void onJsonReceived(const QJsonObject& obj); // 设为公有，便于主界面分发消息

private slots:
    void onConnected();
    void onRefresh();
    void onRowDetailClicked();

private:
    void requestAppointments();
    void openDetailDialog(const QJsonObject& appt);
    void setupUI();

    QString doctorName_;
    CommunicationClient* client_ {nullptr};
    bool ownsClient_ {false}; // 标记是否拥有客户端实例
    QTableWidget* table_ {nullptr};
    QPushButton* refreshBtn_ {nullptr};
};

#endif // APPOINTMENTSWIDGET_H
