#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>

DBManager::DBManager(const QString& path) {
    m_db = QSqlDatabase::addDatabase("QSQLITE", "server_connection");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "Error: connection with database fail";
    } else {
        qDebug() << "Database: connection ok";
    }
    initDatabase();
}

DBManager::~DBManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase("server_connection");
}

void DBManager::initDatabase() {
    if (!m_db.tables().contains(QStringLiteral("users"))) {
        QSqlQuery query(m_db);
        if(!query.exec("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, password TEXT, role TEXT)")) {
            qDebug() << "Couldn't create the table 'users':" << query.lastError();
        }
    }
    if (!m_db.tables().contains(QStringLiteral("doctors"))) {
        QSqlQuery query(m_db);
        if(!query.exec("CREATE TABLE doctors (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, name TEXT, department TEXT, phone TEXT)")) {
            qDebug() << "Couldn't create the table 'doctors':" << query.lastError();
        }
    }
    if (!m_db.tables().contains(QStringLiteral("patients"))) {
        QSqlQuery query(m_db);
        if(!query.exec("CREATE TABLE patients (id INTEGER PRIMARY KEY AUTOINCREMENT, username TEXT UNIQUE, name TEXT, age INTEGER, phone TEXT, address TEXT)")) {
            qDebug() << "Couldn't create the table 'patients':" << query.lastError();
        }
    }
}

bool DBManager::authenticateUser(const QString& username, const QString& password) {
    QSqlQuery query(m_db);
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        return query.value(0).toString() == password;
    }
    return false;
}

bool DBManager::addUser(const QString& username, const QString& password, const QString& role) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":role", role);
    if (!query.exec()) {
        qDebug() << "addUser error:" << query.lastError();
        return false;
    }

    if (role == "doctor") {
        QSqlQuery docQuery(m_db);
        docQuery.prepare("INSERT INTO doctors (username, name) VALUES (:username, :name)");
        docQuery.bindValue(":username", username);
        docQuery.bindValue(":name", username); // Default name to username
        if (!docQuery.exec()) {
            qDebug() << "addUser (doctor) error:" << docQuery.lastError();
            return false;
        }
    } else if (role == "patient") {
        QSqlQuery patQuery(m_db);
        patQuery.prepare("INSERT INTO patients (username, name) VALUES (:username, :name)");
        patQuery.bindValue(":username", username);
        patQuery.bindValue(":name", username); // Default name to username
        if (!patQuery.exec()) {
            qDebug() << "addUser (patient) error:" << patQuery.lastError();
            return false;
        }
    }
    return true;
}

bool DBManager::getDoctorInfo(const QString& username, QJsonObject& doctorInfo) {
    QSqlQuery query(m_db);
    query.prepare("SELECT name, department, phone FROM doctors WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        doctorInfo["name"] = query.value(0).toString();
        doctorInfo["department"] = query.value(1).toString();
        doctorInfo["phone"] = query.value(2).toString();
        return true;
    }
    return false;
}

bool DBManager::getPatientInfo(const QString& username, QJsonObject& patientInfo) {
    QSqlQuery query(m_db);
    query.prepare("SELECT name, age, phone, address FROM patients WHERE username = :username");
    query.bindValue(":username", username);
    if (query.exec() && query.next()) {
        patientInfo["name"] = query.value(0).toString();
        patientInfo["age"] = query.value(1).toInt();
        patientInfo["phone"] = query.value(2).toString();
        patientInfo["address"] = query.value(3).toString();
        return true;
    }
    return false;
}

bool DBManager::updateDoctorInfo(const QString& username, const QJsonObject& data) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE doctors SET name = :name, department = :department, phone = :phone WHERE username = :username");
    query.bindValue(":name", data["name"].toString());
    query.bindValue(":department", data["department"].toString());
    query.bindValue(":phone", data["phone"].toString());
    query.bindValue(":username", username);
    if (!query.exec()) {
        qDebug() << "updateDoctorInfo error:" << query.lastError();
        return false;
    }
    return true;
}

bool DBManager::updatePatientInfo(const QString& username, const QJsonObject& data) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE patients SET name = :name, age = :age, phone = :phone, address = :address WHERE username = :username");
    query.bindValue(":name", data["name"].toString());
    query.bindValue(":age", data["age"].toInt());
    query.bindValue(":phone", data["phone"].toString());
    query.bindValue(":address", data["address"].toString());
    query.bindValue(":username", username);
    if (!query.exec()) {
        qDebug() << "updatePatientInfo error:" << query.lastError();
        return false;
    }
    return true;
}

// ==================== Functions from client ====================

bool DBManager::registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO doctors (name, password, department, phone) VALUES (:name, :password, :department, :phone)");
    query.bindValue(":name", name);
    query.bindValue(":password", password);
    query.bindValue(":department", department);
    query.bindValue(":phone", phone);
    if (!query.exec()) {
        qDebug() << "registerDoctor error:" << query.lastError();
        return false;
    }
    return true;
}

bool DBManager::registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address) {
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO patients (name, password, age, phone, address) VALUES (:name, :password, :age, :phone, :address)");
    query.bindValue(":name", name);
    query.bindValue(":password", password);
    query.bindValue(":age", age);
    query.bindValue(":phone", phone);
    query.bindValue(":address", address);
    if (!query.exec()) {
        qDebug() << "registerPatient error:" << query.lastError();
        return false;
    }
    return true;
}

bool DBManager::loginDoctor(const QString& name, const QString& password) {
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM doctors WHERE name = :name AND password = :password");
    query.bindValue(":name", name);
    query.bindValue(":password", password);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

bool DBManager::loginPatient(const QString& name, const QString& password) {
    QSqlQuery query(m_db);
    query.prepare("SELECT id FROM patients WHERE name = :name AND password = :password");
    query.bindValue(":name", name);
    query.bindValue(":password", password);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

bool DBManager::getDoctorDetails(const QString& name, QString& department, QString& phone) {
    QSqlQuery query(m_db);
    query.prepare("SELECT department, phone FROM doctors WHERE name = :name");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        department = query.value(0).toString();
        phone = query.value(1).toString();
        return true;
    }
    return false;
}

bool DBManager::getPatientDetails(const QString& name, int& age, QString& phone, QString& address) {
    QSqlQuery query(m_db);
    query.prepare("SELECT age, phone, address FROM patients WHERE name = :name");
    query.bindValue(":name", name);
    if (query.exec() && query.next()) {
        age = query.value(0).toInt();
        phone = query.value(1).toString();
        address = query.value(2).toString();
        return true;
    }
    return false;
}

bool DBManager::updateDoctorProfile(const QString& oldName, const QString& newName, const QString& department, const QString& phone) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE doctors SET name = :newName, department = :department, phone = :phone WHERE name = :oldName");
    query.bindValue(":newName", newName);
    query.bindValue(":department", department);
    query.bindValue(":phone", phone);
    query.bindValue(":oldName", oldName);
    if (!query.exec()) {
        qDebug() << "updateDoctorProfile error:" << query.lastError();
        return false;
    }
    return true;
}

bool DBManager::updatePatientProfile(const QString& oldName, const QString& newName, int age, const QString& phone, const QString& address) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE patients SET name = :newName, age = :age, phone = :phone, address = :address WHERE name = :oldName");
    query.bindValue(":newName", newName);
    query.bindValue(":age", age);
    query.bindValue(":phone", phone);
    query.bindValue(":address", address);
    query.bindValue(":oldName", oldName);
    if (!query.exec()) {
        qDebug() << "updatePatientProfile error:" << query.lastError();
        return false;
    }
    return true;
}
