#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QJsonObject>

class DBManager {
public:
    DBManager(const QString& path);
    ~DBManager();

    bool authenticateUser(const QString& username, const QString& password);
    bool addUser(const QString& username, const QString& password, const QString& role);
    bool getDoctorInfo(const QString& username, QJsonObject& doctorInfo);
    bool getPatientInfo(const QString& username, QJsonObject& patientInfo);
    bool updateDoctorInfo(const QString& username, const QJsonObject& data);
    bool updatePatientInfo(const QString& username, const QJsonObject& data);

    // Functions from client
    bool registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone);
    bool registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address);
    bool loginDoctor(const QString& name, const QString& password);
    bool loginPatient(const QString& name, const QString& password);
    bool getDoctorDetails(const QString& name, QString& department, QString& phone);
    bool getPatientDetails(const QString& name, int& age, QString& phone, QString& address);
    bool updateDoctorProfile(const QString& oldName, const QString& newName, const QString& department, const QString& phone);
    bool updatePatientProfile(const QString& oldName, const QString& newName, int age, const QString& phone, const QString& address);

private:
    QSqlDatabase m_db;
    void initDatabase();
};

#endif // DATABASE_H
