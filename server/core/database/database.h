#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QDateTime>

class DBManager {
public:
    DBManager(const QString& path);
    ~DBManager();

    // 原有接口保持兼容
    bool authenticateUser(const QString& username, const QString& password);
    bool addUser(const QString& username, const QString& password, const QString& role);
    bool getDoctorInfo(const QString& username, QJsonObject& doctorInfo);
    bool getPatientInfo(const QString& username, QJsonObject& patientInfo);
    bool updateDoctorInfo(const QString& username, const QJsonObject& data);
    bool updatePatientInfo(const QString& username, const QJsonObject& data);
    bool getUserRole(const QString& username, QString &role);

    // 原有简化接口（保持向后兼容）
    bool registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone);
    bool registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address);
    bool loginDoctor(const QString& name, const QString& password);
    bool loginPatient(const QString& name, const QString& password);
    bool getDoctorDetails(const QString& name, QString& department, QString& phone);
    bool getPatientDetails(const QString& name, int& age, QString& phone, QString& address);
    bool updateDoctorProfile(const QString& oldName, const QString& newName, const QString& department, const QString& phone);
    bool updatePatientProfile(const QString& oldName, const QString& newName, int age, const QString& phone, const QString& address);

    // 新增扩展功能
    bool createAppointment(const QJsonObject& appointmentData);
    bool getAppointmentsByPatient(const QString& patientUsername, QJsonArray& appointments);
    bool getAppointmentsByDoctor(const QString& doctorUsername, QJsonArray& appointments);
    bool updateAppointmentStatus(int appointmentId, const QString& status);
    bool deleteAppointment(int appointmentId);

    // 病例管理
    bool createMedicalRecord(const QJsonObject& recordData);
    bool getMedicalRecordsByPatient(const QString& patientUsername, QJsonArray& records);
    bool getMedicalRecordsByDoctor(const QString& doctorUsername, QJsonArray& records);
    bool updateMedicalRecord(int recordId, const QJsonObject& recordData);

    // 医嘱管理
    bool createMedicalAdvice(const QJsonObject& adviceData);
    bool getMedicalAdviceByRecord(int recordId, QJsonArray& advices);
    bool updateMedicalAdvice(int adviceId, const QJsonObject& adviceData);

    // 处方管理
    bool createPrescription(const QJsonObject& prescriptionData);
    bool addPrescriptionItem(const QJsonObject& itemData);
    bool getPrescriptionsByPatient(const QString& patientUsername, QJsonArray& prescriptions);
    bool getPrescriptionsByDoctor(const QString& doctorUsername, QJsonArray& prescriptions);
    bool getPrescriptionDetails(int prescriptionId, QJsonObject& prescription);

    // 药品管理
    bool addMedication(const QJsonObject& medicationData);
    bool getMedications(QJsonArray& medications);
    bool searchMedications(const QString& keyword, QJsonArray& medications);

    // 统计查询
    bool getDoctorStatistics(const QString& doctorUsername, QJsonObject& statistics);
    bool getPatientStatistics(const QString& patientUsername, QJsonObject& statistics);

    // 获取医生列表（用于患者挂号）
    bool getAllDoctors(QJsonArray& doctors);
    bool getDoctorsByDepartment(const QString& department, QJsonArray& doctors);

    // 新增重载方法
    QString getUserRole(const QString& username);

    // 住院信息
    bool createHospitalization(const QJsonObject &data); // {patient_username, doctor_username, ward_number, bed_number, admission_date}
    bool getHospitalizationsByPatient(const QString &patientUsername, QJsonArray &list); // 查看本人住院记录
    bool getAllHospitalizations(QJsonArray &list); // 管理/医生端汇总
    bool getHospitalizationsByDoctor(const QString &doctorUsername, QJsonArray &list);

private:
    QSqlDatabase m_db;
    void initDatabase();
    
    // 表创建方法
    void createUsersTable();
    void createDoctorsTable();
    void createPatientsTable();
    void createAppointmentsTable();
    void createMedicalRecordsTable();
    void createMedicalAdvicesTable();
    void createPrescriptionsTable();
    void createPrescriptionItemsTable();
    void createMedicationsTable();
    void createDoctorSchedulesTable();
    void createHospitalizationsTable();
    
    // 示例数据插入
    void insertSampleMedications();
    void insertSampleDoctors();
};

#endif // DATABASE_H