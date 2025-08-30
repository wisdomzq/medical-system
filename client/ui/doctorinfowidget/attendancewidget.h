#ifndef ATTENDANCEWIDGET_H
#define ATTENDANCEWIDGET_H

#include <QWidget>
#include <QString>
class QLineEdit;
class QTextEdit;
class QTableWidget;
class CommunicationClient;

class QStackedWidget;
class QPushButton;

class AttendanceWidget : public QWidget {
    Q_OBJECT
public:
    explicit AttendanceWidget(const QString& doctorName, QWidget* parent=nullptr);

signals:
    void backRequested();

private slots:
    void showCheckIn();
    void showLeave();
    void showCancelLeave();
    void doCheckIn();
    void submitLeave();
    void refreshActiveLeaves();
    void cancelSelectedLeave();
    void onConnected();
    void onJsonReceived(const QJsonObject& obj);

private:
    QString doctorName_;
    QStackedWidget* stack_ {nullptr};
    QPushButton* btnCheckIn_ {nullptr};
    QPushButton* btnLeave_ {nullptr};
    QPushButton* btnCancel_ {nullptr};
    QPushButton* btnBack_ {nullptr};

    // 打卡页
    QPushButton* btnDoCheckIn_ {nullptr};

    // 请假页
    QLineEdit* leLeaveDate_ {nullptr};
    QTextEdit* teReason_ {nullptr};
    QPushButton* btnSubmitLeave_ {nullptr};

    // 销假页
    QTableWidget* tblLeaves_ {nullptr};
    QPushButton* btnRefreshLeaves_ {nullptr};
    QPushButton* btnCancelLeave_ {nullptr};

    // 网络
    CommunicationClient* client_ {nullptr};
};

#endif // ATTENDANCEWIDGET_H
