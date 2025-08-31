#ifndef MEDICALCRUDSERVICE_H
#define MEDICALCRUDSERVICE_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

class CommunicationClient;

// 医疗记录/医嘱/处方 相关接口汇总
class MedicalCrudService : public QObject {
    Q_OBJECT
public:
    explicit MedicalCrudService(CommunicationClient* sharedClient, QObject* parent=nullptr);

    // 病历
    void getRecordsByPatient(const QString& patientUsername);
    void getRecordsByAppointment(int appointmentId);
    void createRecord(const QJsonObject& data);
    void updateRecord(int recordId, const QJsonObject& data);

    // 医嘱
    void getAdvicesByRecord(int recordId);
    void createAdvice(const QJsonObject& data);

    // 处方
    void getPrescriptionsByPatient(const QString& patientUsername);
    void createPrescription(const QJsonObject& data);

signals:
    // 病历
    void recordsFetched(const QJsonArray& rows);
    void recordCreated(bool success, const QString& message, int recordId);
    void recordUpdated(bool success, const QString& message);

    // 医嘱
    void advicesFetched(const QJsonArray& rows);
    void adviceCreated(bool success, const QString& message);

    // 处方
    void prescriptionsFetched(const QJsonArray& rows);
    void prescriptionCreated(bool success, const QString& message);

private slots:
    void onJsonReceived(const QJsonObject& obj);

private:
    CommunicationClient* m_client {nullptr};
};

#endif // MEDICALCRUDSERVICE_H
