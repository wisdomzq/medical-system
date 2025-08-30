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

private slots:
    void onConnected();
    void onJsonReceived(const QJsonObject& obj);
    void onRefresh();
    void onRowDetailClicked();

private:
    void requestAppointments();
    void openDetailDialog(const QJsonObject& appt);

    QString doctorName_;
    CommunicationClient* client_ {nullptr};
    QTableWidget* table_ {nullptr};
    QPushButton* refreshBtn_ {nullptr};
};

#endif // APPOINTMENTSWIDGET_H
