#pragma once
#include <sqlite3.h>
#include <string>

// 引入 QString
#include <QString>

class DBManager {
public:
    DBManager(const std::string& dbPath);
    ~DBManager();
    bool initTables();

    // 将函数参数从 std::string 修改为 QString
    bool registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone);
    bool registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address);

    // Add login functions
    bool loginDoctor(const QString& name, const QString& password);
    bool loginPatient(const QString& name, const QString& password);

    // Functions to get and update user details
    bool getDoctorDetails(const QString& name, QString& department, QString& phone);
    bool getPatientDetails(const QString& name, int& age, QString& phone, QString& address);
    bool updateDoctorProfile(const QString& oldName, const QString& newName, const QString& department, const QString& phone);
    bool updatePatientProfile(const QString& oldName, const QString& newName, int age, const QString& phone, const QString& address);

private:
    sqlite3* db;
};