#include "core/services/medicalcrudservice.h"
#include "core/network/communicationclient.h"

MedicalCrudService::MedicalCrudService(CommunicationClient* sharedClient, QObject* parent)
    : QObject(parent), m_client(sharedClient) {
    Q_ASSERT(m_client);
    connect(m_client, &CommunicationClient::jsonReceived, this, &MedicalCrudService::onJsonReceived);
}

// 病历
void MedicalCrudService::getRecordsByPatient(const QString& patientUsername) {
    m_client->sendJson(QJsonObject{{"action","get_medical_records_by_patient"}, {"patient_username", patientUsername}});
}
void MedicalCrudService::createRecord(const QJsonObject& data) {
    m_client->sendJson(QJsonObject{{"action","create_medical_record"}, {"data", data}});
}
void MedicalCrudService::updateRecord(int recordId, const QJsonObject& data) {
    m_client->sendJson(QJsonObject{{"action","update_medical_record"}, {"record_id", recordId}, {"data", data}});
}

// 医嘱
void MedicalCrudService::getAdvicesByRecord(int recordId) {
    m_client->sendJson(QJsonObject{{"action","get_medical_advices_by_record"}, {"record_id", recordId}});
}
void MedicalCrudService::createAdvice(const QJsonObject& data) {
    m_client->sendJson(QJsonObject{{"action","create_medical_advice"}, {"data", data}});
}

// 处方
void MedicalCrudService::getPrescriptionsByPatient(const QString& patientUsername) {
    m_client->sendJson(QJsonObject{{"action","get_prescriptions_by_patient"}, {"patient_username", patientUsername}});
}
void MedicalCrudService::createPrescription(const QJsonObject& data) {
    m_client->sendJson(QJsonObject{{"action","create_prescription"}, {"data", data}});
}

void MedicalCrudService::onJsonReceived(const QJsonObject& obj) {
    const auto type = obj.value("type").toString();
    if (type == "medical_records_response") {
        emit recordsFetched(obj.value("data").toArray());
        return;
    }
    if (type == "create_medical_record_response") {
        const bool ok = obj.value("success").toBool();
        int rid = obj.contains("record_id") ? obj.value("record_id").toInt() : -1;
        emit recordCreated(ok, obj.value("message").toString(), rid);
        return;
    }
    if (type == "update_medical_record_response") {
        emit recordUpdated(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "medical_advices_response") {
        emit advicesFetched(obj.value("data").toArray());
        return;
    }
    if (type == "create_medical_advice_response") {
        emit adviceCreated(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
    if (type == "prescriptions_response") {
        emit prescriptionsFetched(obj.value("data").toArray());
        return;
    }
    if (type == "create_prescription_response") {
        emit prescriptionCreated(obj.value("success").toBool(), obj.value("message").toString());
        return;
    }
}
