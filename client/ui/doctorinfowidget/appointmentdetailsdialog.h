#ifndef APPOINTMENT_DETAILS_DIALOG_H
#define APPOINTMENT_DETAILS_DIALOG_H

#include <QDialog>
#include <QGroupBox>
#include <QJsonObject>
#include <QJsonArray>

class QLineEdit;
class QTextEdit;
class QPushButton;
class QListWidget;
class QTableWidget;
class QButtonGroup;
class QGroupBox;
class QDateEdit;
class CommunicationClient;
class MedicalCrudService;
class HospitalizationService;
class AppointmentService;

class AppointmentDetailsDialog : public QDialog {
    Q_OBJECT
public:
    explicit AppointmentDetailsDialog(const QString& doctorUsername, const QJsonObject& appointment, CommunicationClient* client, QWidget* parent=nullptr);

signals:
    void diagnosisCompleted(int appointmentId);

private slots:
    void onConnected();
    void onSaveRecord();
    void onAddAdvice();
    void onAddPrescription();
    void onCompleteDiagnosis();
    void onAddMedication();
    void onRemoveMedication();
    void onMedicationSelectionChanged();

private:
    void requestExistingRecord();
    void requestAdvices();
    void requestPrescriptions();
    void populateFromRecord(const QJsonObject& record);
    void setupPrescriptionItemsTable();
    void updatePrescriptionItemsTable();
    void showMedicationSelectionDialog();

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
    QButtonGroup* adviceTypeGroup_ {nullptr};
    QButtonGroup* advicePriorityGroup_ {nullptr};
    QTextEdit* adviceContentEdit_ {nullptr};
    QPushButton* adviceAddBtn_ {nullptr};
    QListWidget* advicesList_ {nullptr};
    
    // prescriptions
    QLineEdit* prescriptionNotesEdit_ {nullptr};
    QPushButton* prescriptionAddBtn_ {nullptr};
    QListWidget* prescriptionsList_ {nullptr};
    
    // 处方药品相关UI
    QTableWidget* prescriptionItemsTable_ {nullptr};
    QPushButton* addMedicationBtn_ {nullptr};
    QPushButton* removeMedicationBtn_ {nullptr};
    QJsonArray prescriptionItems_; // 当前处方中的药品项

    // hospitalization
    QGroupBox* hospBox_ {nullptr};
    QButtonGroup* hospYesNoGroup_ {nullptr};
    QWidget* hospDetails_ {nullptr};
    QLineEdit* hospWardEdit_ {nullptr};
    QLineEdit* hospBedEdit_ {nullptr};
    QDateEdit* hospAdmissionDateEdit_ {nullptr};
    // 已移除出院日期与日费用输入框
    QPushButton* hospCreateBtn_ {nullptr};
    HospitalizationService* hospService_ {nullptr};

    CommunicationClient* client_ {nullptr};
    MedicalCrudService* service_ {nullptr};
    AppointmentService* apptService_ {nullptr};
    bool ownsClient_ {false};
    // 保存过程中的去重标志，避免重复弹窗
    bool isSaving_ {false};
};

#endif // APPOINTMENT_DETAILS_DIALOG_H
