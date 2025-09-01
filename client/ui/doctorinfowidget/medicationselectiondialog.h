#ifndef MEDICATIONSELECTIONDIALOG_H
#define MEDICATIONSELECTIONDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonArray>

class QTableWidget;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class CommunicationClient;

class MedicationSelectionDialog : public QDialog {
    Q_OBJECT

public:
    explicit MedicationSelectionDialog(CommunicationClient* client, QWidget* parent = nullptr);
    
    QJsonObject getSelectedMedication() const { return selectedMedication_; }

private slots:
    void onSearchTextChanged();
    void onConfirm();
    void onCancel();
    void onMedicationResponse(const QJsonObject& response);
    void onTableDoubleClick(int row, int column);

private:
    void setupUI();
    void loadMedications();
    void filterMedications();
    void populateTable(const QJsonArray& medications);

    CommunicationClient* client_;
    QJsonArray medications_;
    QJsonArray filteredMedications_;
    QJsonObject selectedMedication_;
    
    QTableWidget* medicationsTable_;
    QLineEdit* searchEdit_;
    QSpinBox* quantitySpinBox_;
    QLineEdit* dosageEdit_;
    QComboBox* frequencyComboBox_;
    QLineEdit* durationEdit_;
    QPushButton* confirmBtn_;
    QPushButton* cancelBtn_;
};

#endif // MEDICATIONSELECTIONDIALOG_H
