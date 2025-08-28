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


private:
    QSqlDatabase m_db;
    void initDatabase();
};

#endif // DATABASE_H
