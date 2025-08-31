#ifndef APPOINTMENTSWIDGET_H
#define APPOINTMENTSWIDGET_H

#include <QWidget>
#include <QString>
#include <QJsonObject>

class QTableWidget;
class QPushButton;
class CommunicationClient;
class AppointmentService;

class AppointmentsWidget : public QWidget {
    Q_OBJECT
public:
    explicit AppointmentsWidget(const QString& doctorName, CommunicationClient* client, QWidget* parent=nullptr);
    
    // 无需外部分发

private slots:
    void onConnected();
    void onRefresh();
    void onRowDetailClicked();

private:
    void requestAppointments();
    void openDetailDialog(const QJsonObject& appt);
    void setupUI();
    void renderAppointments(const QJsonArray& data);
    void showFetchError(const QString& message);

    QString doctorName_;
    CommunicationClient* client_ {nullptr};
    bool ownsClient_ {false};
    QTableWidget* table_ {nullptr};
    QPushButton* refreshBtn_ {nullptr};
    AppointmentService* service_ {nullptr};
};

#endif // APPOINTMENTSWIDGET_H
