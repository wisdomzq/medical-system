#ifndef APPOINTMENT_DETAILS_DIALOG_H
#define APPOINTMENT_DETAILS_DIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QJsonObject>

class QLineEdit;
class QTextEdit;
class QPushButton;
class QListWidget;
class QComboBox;
class CommunicationClient;
class MedicalCrudService;

class AppointmentDetailsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AppointmentDetailsDialog(const QString& doctorUsername, const QJsonObject& appointment, CommunicationClient* client, QWidget* parent=nullptr);

private slots:
    void onConnected();
    void onSaveRecord();
    void onCompleteAppointment();
    void onAddAdvice();
    void onAddPrescription();

private:
    void requestExistingRecord();
    void requestAdvices();
    void requestPrescriptions();
    void populateFromRecord(const QJsonObject& record);
    void updateAppointmentStatus(const QString& status);

    QString doctorUsername_;
    QJsonObject appt_;
    int recordId_ { -1 };

    // UI
    QLineEdit* chiefEdit_ {nullptr};
    QTextEdit* diagnosisEdit_ {nullptr};
    QTextEdit* planEdit_ {nullptr};
    QTextEdit* notesEdit_ {nullptr};
    QPushButton* saveBtn_ {nullptr};
    QPushButton* completeBtn_ {nullptr};
    // advices
    QComboBox* adviceTypeCombo_ {nullptr};
    QComboBox* advicePriorityCombo_ {nullptr};
    QTextEdit* adviceContentEdit_ {nullptr};
    QPushButton* adviceAddBtn_ {nullptr};
    QListWidget* advicesList_ {nullptr};
    
    // prescriptions
    QLineEdit* prescriptionNotesEdit_ {nullptr};
    QPushButton* prescriptionAddBtn_ {nullptr};
    QListWidget* prescriptionsList_ {nullptr};

    CommunicationClient* client_ {nullptr};
    MedicalCrudService* service_ {nullptr};
    bool ownsClient_ {false};
};

#endif // APPOINTMENT_DETAILS_DIALOG_H
