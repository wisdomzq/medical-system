#include "dbmanager.h"
#include <iostream>

// 引入 QByteArray 以便进行 UTF-8 转换
#include <QByteArray>

DBManager::DBManager(const std::string& dbPath) : db(nullptr) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

DBManager::~DBManager() {
    if (db) sqlite3_close(db);
}

bool DBManager::initTables() {
    // 使用更常规的复数表名
    const char* doctorTable =
        "CREATE TABLE IF NOT EXISTS doctors ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE," // 添加 UNIQUE 约束防止重名
        "password TEXT NOT NULL,"
        "department TEXT,"
        "phone TEXT"
        ");";
    const char* patientTable =
        "CREATE TABLE IF NOT EXISTS patients ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL UNIQUE," // 添加 UNIQUE 约束防止重名
        "password TEXT NOT NULL,"
        "age INTEGER,"
        "phone TEXT,"
        "address TEXT"
        ");";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, doctorTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "创建 doctors 表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    if (sqlite3_exec(db, patientTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "创建 patients 表失败: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// ==================== 重写注册函数 ====================

bool DBManager::registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone) {
    // 1. 使用问号 (?) 作为占位符，防止 SQL 注入
    const char* sql = "INSERT INTO doctors (name, password, department, phone) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    // 2. 准备 SQL 语句
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备医生注册语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 3. 将 QString 转换为 UTF-8 编码的字节数组
    QByteArray nameUtf8 = name.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();
    QByteArray departmentUtf8 = department.toUtf8();
    QByteArray phoneUtf8 = phone.toUtf8();

    // 4. 将数据安全地绑定到占位符上
    //    SQLITE_TRANSIENT 告诉 sqlite 自己复制一份数据，这样我们的 QByteArray 就可以安全销毁
    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, departmentUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, phoneUtf8.constData(), -1, SQLITE_TRANSIENT);

    // 5. 执行语句
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    if (!success) {
        std::cerr << "执行医生注册失败: " << sqlite3_errmsg(db) << std::endl;
    }

    // 6. 清理资源
    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address) {
    const char* sql = "INSERT INTO patients (name, password, age, phone, address) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备病人注册语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    QByteArray nameUtf8 = name.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();
    QByteArray phoneUtf8 = phone.toUtf8();
    QByteArray addressUtf8 = address.toUtf8();

    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, age); // 绑定整数
    sqlite3_bind_text(stmt, 4, phoneUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, addressUtf8.constData(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
     if (!success) {
        std::cerr << "执行病人注册失败: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

// ==================== 新增登录函数 ====================

bool DBManager::loginDoctor(const QString& name, const QString& password) {
    const char* sql = "SELECT id FROM doctors WHERE name = ? AND password = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备医生登录语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    QByteArray nameUtf8 = name.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();

    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordUtf8.constData(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::loginPatient(const QString& name, const QString& password) {
    const char* sql = "SELECT id FROM patients WHERE name = ? AND password = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备病人登录语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    QByteArray nameUtf8 = name.toUtf8();
    QByteArray passwordUtf8 = password.toUtf8();

    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordUtf8.constData(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::getDoctorDetails(const QString& name, QString& department, QString& phone) {
    const char* sql = "SELECT department, phone FROM doctors WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    QByteArray nameUtf8 = name.toUtf8();
    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        department = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        phone = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool DBManager::getPatientDetails(const QString& name, int& age, QString& phone, QString& address) {
    const char* sql = "SELECT age, phone, address FROM patients WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    QByteArray nameUtf8 = name.toUtf8();
    sqlite3_bind_text(stmt, 1, nameUtf8.constData(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        age = sqlite3_column_int(stmt, 0);
        phone = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        address = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}

bool DBManager::updateDoctorProfile(const QString& oldName, const QString& newName, const QString& department, const QString& phone) {
    const char* sql = "UPDATE doctors SET name = ?, department = ?, phone = ? WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    QByteArray newNameUtf8 = newName.toUtf8();
    QByteArray departmentUtf8 = department.toUtf8();
    QByteArray phoneUtf8 = phone.toUtf8();
    QByteArray oldNameUtf8 = oldName.toUtf8();

    sqlite3_bind_text(stmt, 1, newNameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, departmentUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, phoneUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, oldNameUtf8.constData(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

bool DBManager::updatePatientProfile(const QString& oldName, const QString& newName, int age, const QString& phone, const QString& address) {
    const char* sql = "UPDATE patients SET name = ?, age = ?, phone = ?, address = ? WHERE name = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    QByteArray newNameUtf8 = newName.toUtf8();
    QByteArray phoneUtf8 = phone.toUtf8();
    QByteArray addressUtf8 = address.toUtf8();
    QByteArray oldNameUtf8 = oldName.toUtf8();

    sqlite3_bind_text(stmt, 1, newNameUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, age);
    sqlite3_bind_text(stmt, 3, phoneUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, addressUtf8.constData(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, oldNameUtf8.constData(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}