#ifndef APPOINTMENT_DETAILS_DIALOG_H
#define APPOINTMENT_DETAILS_DIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QJsonObject>

class QLineEdit;
class QTextEdit;
class QPushButton;
class QListWidget;
class CommunicationClient;

class AppointmentDetailsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AppointmentDetailsDialog(const QString& doctorUsername, const QJsonObject& appointment, QWidget* parent=nullptr);

private slots:
    void onConnected();
    void onJsonReceived(const QJsonObject& obj);
    void onSaveRecord();
    void onAddAdvice();

private:
    void requestExistingRecord();
    void requestAdvices();
    void populateFromRecord(const QJsonObject& record);

    QString doctorUsername_;
    QJsonObject appt_;
    int recordId_ { -1 };

    // UI
    QLineEdit* chiefEdit_ {nullptr};
    QTextEdit* diagnosisEdit_ {nullptr};
    QTextEdit* planEdit_ {nullptr};
    QTextEdit* notesEdit_ {nullptr};
    QPushButton* saveBtn_ {nullptr};
    // advices
    QLineEdit* adviceTypeEdit_ {nullptr};
    QLineEdit* advicePriorityEdit_ {nullptr};
    QTextEdit* adviceContentEdit_ {nullptr};
    QPushButton* adviceAddBtn_ {nullptr};
    QListWidget* advicesList_ {nullptr};

    CommunicationClient* client_ {nullptr};
};

#endif // APPOINTMENT_DETAILS_DIALOG_H
