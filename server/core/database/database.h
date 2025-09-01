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
    
    // 增强的预约排班管理
    bool getDoctorScheduleWithAppointmentStats(const QString& doctorUsername, QJsonArray& scheduleStats);
    bool getAllDoctorsScheduleOverview(QJsonArray& doctorsSchedule);

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
    int createPrescriptionAndGetId(const QJsonObject& prescriptionData);  // 创建处方并返回ID
    bool addPrescriptionItem(const QJsonObject& itemData);
    bool updatePrescriptionStatus(int prescriptionId, const QString& status);  // 更新处方状态
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

    // 住院管理
    bool createHospitalization(const QJsonObject& hospitalizationData);
    bool getHospitalizationsByPatient(const QString& patientUsername, QJsonArray& hospitalizations);
    bool getHospitalizationsByDoctor(const QString& doctorUsername, QJsonArray& hospitalizations);
    bool getAllHospitalizations(QJsonArray& hospitalizations);
    bool updateHospitalizationStatus(int hospitalizationId, const QString& status);
    bool deleteHospitalization(int hospitalizationId);

    // 获取医生列表（用于患者挂号）
    bool getAllDoctors(QJsonArray& doctors);
    bool getDoctorsByDepartment(const QString& department, QJsonArray& doctors);
    
    // 医生排班管理
    bool getDoctorSchedules(const QString& doctorUsername, QJsonArray& schedules);

    // 新增重载方法
    QString getUserRole(const QString& username);

    // 住院信息（统一采用上方“住院管理”的新接口）

    // 考勤与请假
    bool createAttendanceRecord(const QJsonObject &data); // {doctor_username, checkin_date, checkin_time}
    bool createLeaveRequest(const QJsonObject &data);     // {doctor_username, leave_date, reason}
    bool getActiveLeavesByDoctor(const QString &doctorUsername, QJsonArray &leaves); // status='active'
    bool cancelLeaveById(int leaveId);                    // 将 status 置为 'cancelled'
    bool cancelActiveLeaveForDoctor(const QString &doctorUsername); // 取消最近一条 active
    // 考勤查询
    bool getAttendanceByDoctor(const QString &doctorUsername, QJsonArray &records, int limit = 100);
    
    // 工具方法
    int getLastInsertId();  // 获取最后插入记录的ID

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
    void createAttendanceTable();
    void createLeaveRequestsTable();
    
    // 示例数据插入
    void insertSampleMedications();
    void insertSampleDoctors();
    void insertSampleDoctorSchedules();
};

#endif // DATABASE_H