#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QJsonArray>

DBManager::DBManager(const QString& path) {
    // 使用唯一的连接名称，避免与TcpServer冲突
    static int connectionId = 0;
    QString connectionName = QString("main_db_connection_%1").arg(++connectionId);
    
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "Error: connection with database fail" << m_db.lastError().text();
    } else {
        qDebug() << "Database: connection ok";
        // 启用外键约束
        QSqlQuery query(m_db);
        if (!query.exec("PRAGMA foreign_keys = ON;")) {
            qDebug() << "启用外键约束失败:" << query.lastError().text();
        }
    }
    initDatabase();
}

DBManager::~DBManager() {
    QString connectionName = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(connectionName); // 使用正确的连接名称
}

void DBManager::initDatabase() {
    createUsersTable();
    createDoctorsTable();
    createPatientsTable();
    createAppointmentsTable();
    createMedicalRecordsTable();
    createMedicalAdvicesTable();
    createPrescriptionsTable();
    createPrescriptionItemsTable();
    createMedicationsTable();
    createDoctorSchedulesTable();
    createHospitalizationsTable();
    
    // 清理不一致的数据
    cleanupInconsistentData();
    
    // 插入示例数据
    insertSampleDoctors();
}

void DBManager::createUsersTable() {
    if (!m_db.tables().contains(QStringLiteral("users"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                role TEXT NOT NULL CHECK(role IN ('doctor', 'patient')),
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建users表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createDoctorsTable() {
    if (!m_db.tables().contains(QStringLiteral("doctors"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE doctors (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                name TEXT NOT NULL,
                department TEXT,
                phone TEXT,
                email TEXT,
                work_number TEXT,
                title TEXT,
                specialization TEXT,
                consultation_fee DECIMAL(10,2) DEFAULT 0.00,
                max_patients_per_day INTEGER DEFAULT 20,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建doctors表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createPatientsTable() {
    if (!m_db.tables().contains(QStringLiteral("patients"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE patients (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                name TEXT NOT NULL,
                age INTEGER,
                gender TEXT CHECK(gender IN ('男', '女')),
                phone TEXT,
                email TEXT,
                address TEXT,
                id_card TEXT,
                emergency_contact TEXT,
                emergency_phone TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建patients表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createAppointmentsTable() {
    if (!m_db.tables().contains(QStringLiteral("appointments"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE appointments (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                patient_username TEXT NOT NULL,
                doctor_username TEXT NOT NULL,
                appointment_date DATE NOT NULL,
                appointment_time TIME NOT NULL,
                status TEXT DEFAULT 'pending' CHECK(status IN ('pending', 'confirmed', 'completed', 'cancelled')),
                department TEXT,
                chief_complaint TEXT,
                fee DECIMAL(10,2),
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (patient_username) REFERENCES users(username),
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建appointments表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createMedicalRecordsTable() {
    if (!m_db.tables().contains(QStringLiteral("medical_records"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE medical_records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                appointment_id INTEGER,
                patient_username TEXT NOT NULL,
                doctor_username TEXT NOT NULL,
                visit_date DATETIME NOT NULL,
                chief_complaint TEXT,
                present_illness TEXT,
                past_history TEXT,
                physical_examination TEXT,
                diagnosis TEXT,
                treatment_plan TEXT,
                notes TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (appointment_id) REFERENCES appointments(id),
                FOREIGN KEY (patient_username) REFERENCES users(username),
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建medical_records表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createMedicalAdvicesTable() {
    if (!m_db.tables().contains(QStringLiteral("medical_advices"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE medical_advices (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                record_id INTEGER NOT NULL,
                advice_type TEXT NOT NULL CHECK(advice_type IN ('medication', 'lifestyle', 'followup', 'examination')),
                content TEXT NOT NULL,
                priority TEXT DEFAULT 'normal' CHECK(priority IN ('low', 'normal', 'high', 'urgent')),
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (record_id) REFERENCES medical_records(id)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建medical_advices表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createPrescriptionsTable() {
    if (!m_db.tables().contains(QStringLiteral("prescriptions"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE prescriptions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                record_id INTEGER,
                patient_username TEXT NOT NULL,
                doctor_username TEXT NOT NULL,
                prescription_date DATETIME NOT NULL,
                total_amount DECIMAL(10,2) DEFAULT 0.00,
                status TEXT DEFAULT 'pending' CHECK(status IN ('pending', 'dispensed', 'cancelled')),
                notes TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (record_id) REFERENCES medical_records(id),
                FOREIGN KEY (patient_username) REFERENCES users(username),
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建prescriptions表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createPrescriptionItemsTable() {
    if (!m_db.tables().contains(QStringLiteral("prescription_items"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE prescription_items (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                prescription_id INTEGER NOT NULL,
                medication_id INTEGER NOT NULL,
                quantity INTEGER NOT NULL,
                dosage TEXT,
                frequency TEXT,
                duration TEXT,
                instructions TEXT,
                unit_price DECIMAL(10,2),
                total_price DECIMAL(10,2),
                FOREIGN KEY (prescription_id) REFERENCES prescriptions(id),
                FOREIGN KEY (medication_id) REFERENCES medications(id)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建prescription_items表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createMedicationsTable() {
    if (!m_db.tables().contains(QStringLiteral("medications"))) {
        QSqlQuery query(m_db);
    QString sql = R"(
            CREATE TABLE medications (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                generic_name TEXT,
                category TEXT,
                manufacturer TEXT,
                specification TEXT,
                unit TEXT,
                price DECIMAL(10,2),
                stock_quantity INTEGER DEFAULT 0,
                description TEXT,
                precautions TEXT,
                side_effects TEXT,
                contraindications TEXT,
                image_path TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建medications表失败:" << query.lastError().text();
        }
        
        // 插入一些示例药品数据
        insertSampleMedications();
    }
}

void DBManager::createDoctorSchedulesTable() {
    if (!m_db.tables().contains(QStringLiteral("doctor_schedules"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE doctor_schedules (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                doctor_username TEXT NOT NULL,
                day_of_week INTEGER NOT NULL CHECK(day_of_week BETWEEN 0 AND 6),
                start_time TIME NOT NULL,
                end_time TIME NOT NULL,
                max_appointments INTEGER DEFAULT 10,
                is_active BOOLEAN DEFAULT 1,
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建doctor_schedules表失败:" << query.lastError().text();
        }
    }
}

void DBManager::createHospitalizationsTable() {
    if (!m_db.tables().contains(QStringLiteral("hospitalizations"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE hospitalizations (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                patient_username TEXT NOT NULL,
                doctor_username TEXT NOT NULL,
                ward_number TEXT,
                bed_number TEXT,
                admission_date DATE NOT NULL,
                discharge_date DATE,
                status TEXT DEFAULT 'active' CHECK(status IN ('active','discharged')),
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (patient_username) REFERENCES users(username),
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建hospitalizations表失败:" << query.lastError().text();
        }
    }
}

bool DBManager::createHospitalization(const QJsonObject &data) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO hospitalizations (patient_username, doctor_username, ward_number, bed_number, admission_date, discharge_date, status)
                VALUES (:p, :d, :w, :b, :ad, :dd, :st))");
    q.bindValue(":p", data.value("patient_username").toString());
    q.bindValue(":d", data.value("doctor_username").toString());
    q.bindValue(":w", data.value("ward_number").toString());
    q.bindValue(":b", data.value("bed_number").toString());
    q.bindValue(":ad", data.value("admission_date").toString());
    q.bindValue(":dd", data.value("discharge_date").toString());
    q.bindValue(":st", data.value("status").toString().isEmpty()? QString("active") : data.value("status").toString());
    if(!q.exec()) { qDebug() << "createHospitalization error:" << q.lastError().text(); return false; }
    return true;
}

static bool fetchHospitalizations(QSqlDatabase &db, QSqlQuery &q, QJsonArray &list) {
    if(!q.exec()) { qDebug() << "fetchHospitalizations query exec error:" << q.lastError().text(); return false; }
    while(q.next()) {
        QJsonObject o; o["id"]=q.value("id").toInt(); o["patient_username"]=q.value("patient_username").toString(); o["doctor_username"]=q.value("doctor_username").toString(); o["ward_number"]=q.value("ward_number").toString(); o["bed_number"]=q.value("bed_number").toString(); o["admission_date"]=q.value("admission_date").toString(); o["discharge_date"]=q.value("discharge_date").toString(); o["status"]=q.value("status").toString(); list.append(o); }
    return true;
}

bool DBManager::getHospitalizationsByPatient(const QString &patientUsername, QJsonArray &list) {
    QSqlQuery q(m_db); q.prepare("SELECT * FROM hospitalizations WHERE patient_username=:p ORDER BY admission_date DESC"); q.bindValue(":p", patientUsername); return fetchHospitalizations(m_db,q,list);
}

bool DBManager::getAllHospitalizations(QJsonArray &list) {
    QSqlQuery q(m_db); q.prepare("SELECT * FROM hospitalizations ORDER BY admission_date DESC"); return fetchHospitalizations(m_db,q,list);
}

bool DBManager::getHospitalizationsByDoctor(const QString &doctorUsername, QJsonArray &list) {
    QSqlQuery q(m_db); q.prepare("SELECT * FROM hospitalizations WHERE doctor_username=:d ORDER BY admission_date DESC"); q.bindValue(":d", doctorUsername); return fetchHospitalizations(m_db,q,list);
}

bool DBManager::authenticateUser(const QString& username, const QString& password) {
    QSqlQuery query(m_db);
    query.prepare("SELECT username FROM users WHERE username = :username AND password = :password");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    
    if (!query.exec()) {
        qDebug() << "authenticateUser error:" << query.lastError().text();
        return false;
    }
    
    return query.next();
}

bool DBManager::getUserRole(const QString &username, QString &role) {
    QSqlQuery query(m_db);
    query.prepare("SELECT role FROM users WHERE username = :u");
    query.bindValue(":u", username);
    if (query.exec() && query.next()) {
        role = query.value(0).toString();
        return true;
    }
    return false;
}

// ==================== 修改开始 ====================

// 修改 addUser 函数，使其只负责向 users 表添加用户。
// 这是更清晰的单一职责设计。
bool DBManager::addUser(const QString& username, const QString& password, const QString& role) {
    // 首先检查用户名是否已存在
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT COUNT(*) FROM users WHERE username = :username");
    checkQuery.bindValue(":username", username);
    if (checkQuery.exec() && checkQuery.next()) {
        int count = checkQuery.value(0).toInt();
        if (count > 0) {
            qDebug() << "addUser error: Username already exists:" << username;
            return false; // 用户名已存在
        }
    }

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":role", role);
    if (!query.exec()) {
        qDebug() << "addUser error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getDoctorInfo(const QString& username, QJsonObject& doctorInfo) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT name, department, phone, email, work_number, title, 
               specialization, consultation_fee, max_patients_per_day
        FROM doctors WHERE username = :username
    )");
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        doctorInfo["name"] = query.value("name").toString();
        doctorInfo["department"] = query.value("department").toString();
        doctorInfo["phone"] = query.value("phone").toString();
        doctorInfo["email"] = query.value("email").toString();
        doctorInfo["work_number"] = query.value("work_number").toString();
        doctorInfo["title"] = query.value("title").toString();
        doctorInfo["specialization"] = query.value("specialization").toString();
        doctorInfo["consultation_fee"] = query.value("consultation_fee").toDouble();
        doctorInfo["max_patients_per_day"] = query.value("max_patients_per_day").toInt();
        return true;
    }
    qDebug() << "getDoctorInfo error:" << query.lastError().text();
    return false;
}

bool DBManager::getPatientInfo(const QString& username, QJsonObject& patientInfo) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT name, age, gender, phone, email, address, id_card, 
               emergency_contact, emergency_phone
        FROM patients WHERE username = :username
    )");
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        patientInfo["name"] = query.value("name").toString();
        patientInfo["age"] = query.value("age").toInt();
        patientInfo["gender"] = query.value("gender").toString();
        patientInfo["phone"] = query.value("phone").toString();
        patientInfo["email"] = query.value("email").toString();
        patientInfo["address"] = query.value("address").toString();
        patientInfo["id_card"] = query.value("id_card").toString();
        patientInfo["emergency_contact"] = query.value("emergency_contact").toString();
        patientInfo["emergency_phone"] = query.value("emergency_phone").toString();
        return true;
    }
    qDebug() << "getPatientInfo error:" << query.lastError().text();
    return false;
}

bool DBManager::updateDoctorInfo(const QString& username, const QJsonObject& data) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE doctors SET 
            name = :name, 
            department = :department, 
            phone = :phone, 
            email = :email,
            work_number = :work_number,
            title = :title,
            specialization = :specialization,
            consultation_fee = :consultation_fee,
            max_patients_per_day = :max_patients_per_day,
            updated_at = CURRENT_TIMESTAMP
        WHERE username = :username
    )");
    
    query.bindValue(":name", data["name"].toString());
    query.bindValue(":department", data["department"].toString());
    query.bindValue(":phone", data["phone"].toString());
    query.bindValue(":email", data["email"].toString());
    query.bindValue(":work_number", data["work_number"].toString());
    query.bindValue(":title", data["title"].toString());
    query.bindValue(":specialization", data["specialization"].toString());
    query.bindValue(":consultation_fee", data["consultation_fee"].toDouble());
    query.bindValue(":max_patients_per_day", data["max_patients_per_day"].toInt());
    query.bindValue(":username", username);
    
    if (!query.exec()) {
        qDebug() << "updateDoctorInfo error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::updatePatientInfo(const QString& username, const QJsonObject& data) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE patients SET 
            name = :name, 
            age = :age, 
            gender = :gender,
            phone = :phone, 
            email = :email,
            address = :address,
            id_card = :id_card,
            emergency_contact = :emergency_contact,
            emergency_phone = :emergency_phone,
            updated_at = CURRENT_TIMESTAMP
        WHERE username = :username
    )");
    
    query.bindValue(":name", data["name"].toString());
    query.bindValue(":age", data["age"].toInt());
    query.bindValue(":gender", data["gender"].toString());
    query.bindValue(":phone", data["phone"].toString());
    query.bindValue(":email", data["email"].toString());
    query.bindValue(":address", data["address"].toString());
    query.bindValue(":id_card", data["id_card"].toString());
    query.bindValue(":emergency_contact", data["emergency_contact"].toString());
    query.bindValue(":emergency_phone", data["emergency_phone"].toString());
    query.bindValue(":username", username);
    
    if (!query.exec()) {
        qDebug() << "updatePatientInfo error:" << query.lastError().text();
        return false;
    }
    return true;
}

// ==================== Functions from client ====================
// 重写 registerDoctor，使用事务确保原子性，并一次性插入所有数据。
bool DBManager::registerDoctor(const QString& name, const QString& password, const QString& department, const QString& phone) {
    qDebug() << "开始注册医生:" << name;
    
    // 开启数据库事务
    if (!m_db.transaction()) {
        qDebug() << "Failed to start transaction.";
        return false;
    }

    // 1. 插入到 users 表
    if (!addUser(name, password, "doctor")) {
        qDebug() << "注册医生失败: 用户名已存在或添加用户失败:" << name;
        m_db.rollback(); // 如果失败，回滚事务
        return false;
    }

    // 2. 一次性插入完整信息到 doctors 表
    QSqlQuery docQuery(m_db);
    docQuery.prepare("INSERT INTO doctors (username, name, department, phone) VALUES (:username, :name, :department, :phone)");
    docQuery.bindValue(":username", name);
    docQuery.bindValue(":name", name); // 默认 name 和 username 相同
    docQuery.bindValue(":department", department);
    docQuery.bindValue(":phone", phone);
    
    if (!docQuery.exec()) {
        qDebug() << "registerDoctor insert error:" << docQuery.lastError().text();
        m_db.rollback(); // 如果失败，回滚事务
        return false;
    }

    // 提交事务
    if (!m_db.commit()) {
        qDebug() << "Failed to commit transaction.";
        m_db.rollback();
        return false;
    }
    
    qDebug() << "医生注册成功:" << name;
    return true;
}

// 重写 registerPatient，逻辑同上。
bool DBManager::registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address) {
    qDebug() << "开始注册病人:" << name;
    
    if (!m_db.transaction()) {
        qDebug() << "Failed to start transaction.";
        return false;
    }

    // 1. 插入到 users 表
    if (!addUser(name, password, "patient")) {
        qDebug() << "注册病人失败: 用户名已存在或添加用户失败:" << name;
        m_db.rollback();
        return false;
    }

    // 2. 一次性插入完整信息到 patients 表
    QSqlQuery patQuery(m_db);
    patQuery.prepare("INSERT INTO patients (username, name, age, phone, address) VALUES (:username, :name, :age, :phone, :address)");
    patQuery.bindValue(":username", name);
    patQuery.bindValue(":name", name); // 默认 name 和 username 相同
    patQuery.bindValue(":age", age);
    patQuery.bindValue(":phone", phone);
    patQuery.bindValue(":address", address);
    
    if (!patQuery.exec()) {
        qDebug() << "registerPatient insert error:" << patQuery.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        qDebug() << "Failed to commit transaction.";
        m_db.rollback();
        return false;
    }
    
    qDebug() << "病人注册成功:" << name;
    return true;
}

bool DBManager::loginDoctor(const QString& name, const QString& password) {
    if (!authenticateUser(name, password)) return false;
    QString role; return getUserRole(name, role) && role == "doctor";
}

bool DBManager::loginPatient(const QString& name, const QString& password) {
    if (!authenticateUser(name, password)) return false;
    QString role; return getUserRole(name, role) && role == "patient";
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
        qDebug() << "updateDoctorProfile error:" << query.lastError().text();
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
        qDebug() << "updatePatientProfile error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 预约管理实现
bool DBManager::createAppointment(const QJsonObject& appointmentData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO appointments (
            patient_username, doctor_username, appointment_date, 
            appointment_time, department, chief_complaint, fee
        ) VALUES (
            :patient_username, :doctor_username, :appointment_date,
            :appointment_time, :department, :chief_complaint, :fee
        )
    )");
    
    query.bindValue(":patient_username", appointmentData["patient_username"].toString());
    query.bindValue(":doctor_username", appointmentData["doctor_username"].toString());
    query.bindValue(":appointment_date", appointmentData["appointment_date"].toString());
    query.bindValue(":appointment_time", appointmentData["appointment_time"].toString());
    query.bindValue(":department", appointmentData["department"].toString());
    query.bindValue(":chief_complaint", appointmentData["chief_complaint"].toString());
    query.bindValue(":fee", appointmentData["fee"].toDouble());
    
    if (!query.exec()) {
        qDebug() << "createAppointment error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getAppointmentsByPatient(const QString& patientUsername, QJsonArray& appointments) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT a.id, a.doctor_username, a.appointment_date, a.appointment_time,
               a.status, a.department, a.chief_complaint, a.fee,
               d.name as doctor_name, d.title as doctor_title
        FROM appointments a
        LEFT JOIN doctors d ON a.doctor_username = d.username
        WHERE a.patient_username = :patient_username
        ORDER BY a.appointment_date DESC, a.appointment_time DESC
    )");
    query.bindValue(":patient_username", patientUsername);

    if (!query.exec()) {
        qDebug() << "getAppointmentsByPatient error:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QJsonObject appointment;
        appointment["id"] = query.value("id").toInt();
        appointment["doctor_username"] = query.value("doctor_username").toString();
        appointment["appointment_date"] = query.value("appointment_date").toString();
        appointment["appointment_time"] = query.value("appointment_time").toString();
        appointment["status"] = query.value("status").toString();
        appointment["department"] = query.value("department").toString();
        appointment["chief_complaint"] = query.value("chief_complaint").toString();
        appointment["fee"] = query.value("fee").toDouble();
        appointment["doctor_name"] = query.value("doctor_name").toString();
        appointment["doctor_title"] = query.value("doctor_title").toString();
        appointments.append(appointment);
    }
    return true;
}

bool DBManager::getAppointmentsByDoctor(const QString& doctorUsername, QJsonArray& appointments) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT a.id, a.patient_username, a.appointment_date, a.appointment_time,
               a.status, a.department, a.chief_complaint, a.fee,
               p.name as patient_name, p.age as patient_age
        FROM appointments a
        LEFT JOIN patients p ON a.patient_username = p.username
        WHERE a.doctor_username = :doctor_username
        ORDER BY a.appointment_date DESC, a.appointment_time DESC
    )");
    query.bindValue(":doctor_username", doctorUsername);

    if (!query.exec()) {
        qDebug() << "getAppointmentsByDoctor error:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QJsonObject appointment;
        appointment["id"] = query.value("id").toInt();
        appointment["patient_username"] = query.value("patient_username").toString();
        appointment["appointment_date"] = query.value("appointment_date").toString();
        appointment["appointment_time"] = query.value("appointment_time").toString();
        appointment["status"] = query.value("status").toString();
        appointment["department"] = query.value("department").toString();
        appointment["chief_complaint"] = query.value("chief_complaint").toString();
        appointment["fee"] = query.value("fee").toDouble();
        appointment["patient_name"] = query.value("patient_name").toString();
        appointment["patient_age"] = query.value("patient_age").toInt();
        appointments.append(appointment);
    }
    return true;
}

bool DBManager::updateAppointmentStatus(int appointmentId, const QString& status) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE appointments SET status = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", appointmentId);
    
    if (!query.exec()) {
        qDebug() << "updateAppointmentStatus error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteAppointment(int appointmentId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM appointments WHERE id = :id");
    query.bindValue(":id", appointmentId);
    
    if (!query.exec()) {
        qDebug() << "deleteAppointment error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 病例管理实现 - 提供基础实现
bool DBManager::createMedicalRecord(const QJsonObject& recordData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO medical_records (
            appointment_id, patient_username, doctor_username, visit_date,
            chief_complaint, present_illness, past_history, physical_examination,
            diagnosis, treatment_plan, notes
        ) VALUES (
            :appointment_id, :patient_username, :doctor_username, :visit_date,
            :chief_complaint, :present_illness, :past_history, :physical_examination,
            :diagnosis, :treatment_plan, :notes
        )
    )");
    
    query.bindValue(":appointment_id", recordData["appointment_id"].toInt());
    query.bindValue(":patient_username", recordData["patient_username"].toString());
    query.bindValue(":doctor_username", recordData["doctor_username"].toString());
    query.bindValue(":visit_date", recordData["visit_date"].toString());
    query.bindValue(":chief_complaint", recordData["chief_complaint"].toString());
    query.bindValue(":present_illness", recordData["present_illness"].toString());
    query.bindValue(":past_history", recordData["past_history"].toString());
    query.bindValue(":physical_examination", recordData["physical_examination"].toString());
    query.bindValue(":diagnosis", recordData["diagnosis"].toString());
    query.bindValue(":treatment_plan", recordData["treatment_plan"].toString());
    query.bindValue(":notes", recordData["notes"].toString());
    
    if (!query.exec()) {
        qDebug() << "createMedicalRecord error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getMedicalRecordsByPatient(const QString& patientUsername, QJsonArray& records) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT mr.id, mr.appointment_id, mr.doctor_username, mr.visit_date,
               mr.chief_complaint, mr.present_illness, mr.past_history,
               mr.physical_examination, mr.diagnosis, mr.treatment_plan, mr.notes,
               d.name as doctor_name, d.title as doctor_title, d.department
        FROM medical_records mr
        LEFT JOIN doctors d ON mr.doctor_username = d.username
        WHERE mr.patient_username = :patient_username
        ORDER BY mr.visit_date DESC
    )");
    query.bindValue(":patient_username", patientUsername);
    
    if (!query.exec()) {
        qDebug() << "getMedicalRecordsByPatient error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject record;
        record["id"] = query.value("id").toInt();
        record["appointment_id"] = query.value("appointment_id").toInt();
        record["doctor_username"] = query.value("doctor_username").toString();
        record["visit_date"] = query.value("visit_date").toString();
        record["chief_complaint"] = query.value("chief_complaint").toString();
        record["present_illness"] = query.value("present_illness").toString();
        record["past_history"] = query.value("past_history").toString();
        record["physical_examination"] = query.value("physical_examination").toString();
        record["diagnosis"] = query.value("diagnosis").toString();
        record["treatment_plan"] = query.value("treatment_plan").toString();
        record["notes"] = query.value("notes").toString();
        record["doctor_name"] = query.value("doctor_name").toString();
        record["doctor_title"] = query.value("doctor_title").toString();
        record["department"] = query.value("department").toString();
        records.append(record);
    }
    return true;
}

bool DBManager::getMedicalRecordsByDoctor(const QString& doctorUsername, QJsonArray& records) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT mr.id, mr.appointment_id, mr.patient_username, mr.visit_date,
               mr.chief_complaint, mr.present_illness, mr.past_history,
               mr.physical_examination, mr.diagnosis, mr.treatment_plan, mr.notes,
               p.name as patient_name, p.age as patient_age, p.gender
        FROM medical_records mr
        LEFT JOIN patients p ON mr.patient_username = p.username
        WHERE mr.doctor_username = :doctor_username
        ORDER BY mr.visit_date DESC
    )");
    query.bindValue(":doctor_username", doctorUsername);
    
    if (!query.exec()) {
        qDebug() << "getMedicalRecordsByDoctor error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject record;
        record["id"] = query.value("id").toInt();
        record["appointment_id"] = query.value("appointment_id").toInt();
        record["patient_username"] = query.value("patient_username").toString();
        record["visit_date"] = query.value("visit_date").toString();
        record["chief_complaint"] = query.value("chief_complaint").toString();
        record["present_illness"] = query.value("present_illness").toString();
        record["past_history"] = query.value("past_history").toString();
        record["physical_examination"] = query.value("physical_examination").toString();
        record["diagnosis"] = query.value("diagnosis").toString();
        record["treatment_plan"] = query.value("treatment_plan").toString();
        record["notes"] = query.value("notes").toString();
        record["patient_name"] = query.value("patient_name").toString();
        record["patient_age"] = query.value("patient_age").toInt();
        record["patient_gender"] = query.value("gender").toString();
        records.append(record);
    }
    return true;
}

bool DBManager::updateMedicalRecord(int recordId, const QJsonObject& recordData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE medical_records SET 
            chief_complaint = :chief_complaint,
            present_illness = :present_illness,
            past_history = :past_history,
            physical_examination = :physical_examination,
            diagnosis = :diagnosis,
            treatment_plan = :treatment_plan,
            notes = :notes,
            updated_at = CURRENT_TIMESTAMP
        WHERE id = :id
    )");
    
    query.bindValue(":chief_complaint", recordData["chief_complaint"].toString());
    query.bindValue(":present_illness", recordData["present_illness"].toString());
    query.bindValue(":past_history", recordData["past_history"].toString());
    query.bindValue(":physical_examination", recordData["physical_examination"].toString());
    query.bindValue(":diagnosis", recordData["diagnosis"].toString());
    query.bindValue(":treatment_plan", recordData["treatment_plan"].toString());
    query.bindValue(":notes", recordData["notes"].toString());
    query.bindValue(":id", recordId);
    
    if (!query.exec()) {
        qDebug() << "updateMedicalRecord error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 医嘱管理实现 - 提供基础实现
bool DBManager::createMedicalAdvice(const QJsonObject& adviceData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO medical_advices (
            record_id, advice_type, content, priority
        ) VALUES (
            :record_id, :advice_type, :content, :priority
        )
    )");
    
    query.bindValue(":record_id", adviceData["record_id"].toInt());
    query.bindValue(":advice_type", adviceData["advice_type"].toString());
    query.bindValue(":content", adviceData["content"].toString());
    query.bindValue(":priority", adviceData["priority"].toString());
    
    if (!query.exec()) {
        qDebug() << "createMedicalAdvice error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getMedicalAdviceByRecord(int recordId, QJsonArray& advices) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, advice_type, content, priority, created_at
        FROM medical_advices
        WHERE record_id = :record_id
        ORDER BY created_at DESC
    )");
    query.bindValue(":record_id", recordId);
    
    if (!query.exec()) {
        qDebug() << "getMedicalAdviceByRecord error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject advice;
        advice["id"] = query.value("id").toInt();
        advice["advice_type"] = query.value("advice_type").toString();
        advice["content"] = query.value("content").toString();
        advice["priority"] = query.value("priority").toString();
        advice["created_at"] = query.value("created_at").toString();
        advices.append(advice);
    }
    return true;
}

bool DBManager::updateMedicalAdvice(int adviceId, const QJsonObject& adviceData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE medical_advices SET 
            advice_type = :advice_type,
            content = :content,
            priority = :priority
        WHERE id = :id
    )");
    
    query.bindValue(":advice_type", adviceData["advice_type"].toString());
    query.bindValue(":content", adviceData["content"].toString());
    query.bindValue(":priority", adviceData["priority"].toString());
    query.bindValue(":id", adviceId);
    
    if (!query.exec()) {
        qDebug() << "updateMedicalAdvice error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 处方管理实现 - 提供基础实现
bool DBManager::createPrescription(const QJsonObject& prescriptionData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO prescriptions (
            record_id, patient_username, doctor_username, prescription_date,
            total_amount, status, notes
        ) VALUES (
            :record_id, :patient_username, :doctor_username, :prescription_date,
            :total_amount, :status, :notes
        )
    )");
    
    query.bindValue(":record_id", prescriptionData["record_id"].toInt());
    query.bindValue(":patient_username", prescriptionData["patient_username"].toString());
    query.bindValue(":doctor_username", prescriptionData["doctor_username"].toString());
    query.bindValue(":prescription_date", prescriptionData["prescription_date"].toString());
    query.bindValue(":total_amount", prescriptionData["total_amount"].toDouble());
    query.bindValue(":status", prescriptionData["status"].toString());
    query.bindValue(":notes", prescriptionData["notes"].toString());
    
    if (!query.exec()) {
        qDebug() << "createPrescription error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::addPrescriptionItem(const QJsonObject& itemData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO prescription_items (
            prescription_id, medication_id, quantity, dosage,
            frequency, duration, instructions, unit_price, total_price
        ) VALUES (
            :prescription_id, :medication_id, :quantity, :dosage,
            :frequency, :duration, :instructions, :unit_price, :total_price
        )
    )");
    
    query.bindValue(":prescription_id", itemData["prescription_id"].toInt());
    query.bindValue(":medication_id", itemData["medication_id"].toInt());
    query.bindValue(":quantity", itemData["quantity"].toInt());
    query.bindValue(":dosage", itemData["dosage"].toString());
    query.bindValue(":frequency", itemData["frequency"].toString());
    query.bindValue(":duration", itemData["duration"].toString());
    query.bindValue(":instructions", itemData["instructions"].toString());
    query.bindValue(":unit_price", itemData["unit_price"].toDouble());
    query.bindValue(":total_price", itemData["total_price"].toDouble());
    
    if (!query.exec()) {
        qDebug() << "addPrescriptionItem error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getPrescriptionsByPatient(const QString& patientUsername, QJsonArray& prescriptions) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT p.id, p.record_id, p.doctor_username, p.prescription_date,
               p.total_amount, p.status, p.notes,
               d.name as doctor_name, d.title as doctor_title
        FROM prescriptions p
        LEFT JOIN doctors d ON p.doctor_username = d.username
        WHERE p.patient_username = :patient_username
        ORDER BY p.prescription_date DESC
    )");
    query.bindValue(":patient_username", patientUsername);
    
    if (!query.exec()) {
        qDebug() << "getPrescriptionsByPatient error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject prescription;
        prescription["id"] = query.value("id").toInt();
        prescription["record_id"] = query.value("record_id").toInt();
        prescription["doctor_username"] = query.value("doctor_username").toString();
        prescription["prescription_date"] = query.value("prescription_date").toString();
        prescription["total_amount"] = query.value("total_amount").toDouble();
        prescription["status"] = query.value("status").toString();
        prescription["notes"] = query.value("notes").toString();
        prescription["doctor_name"] = query.value("doctor_name").toString();
        prescription["doctor_title"] = query.value("doctor_title").toString();
        prescriptions.append(prescription);
    }
    return true;
}

bool DBManager::getPrescriptionsByDoctor(const QString& doctorUsername, QJsonArray& prescriptions) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT p.id, p.record_id, p.patient_username, p.prescription_date,
               p.total_amount, p.status, p.notes,
               pt.name as patient_name, pt.age as patient_age
        FROM prescriptions p
        LEFT JOIN patients pt ON p.patient_username = pt.username
        WHERE p.doctor_username = :doctor_username
        ORDER BY p.prescription_date DESC
    )");
    query.bindValue(":doctor_username", doctorUsername);
    
    if (!query.exec()) {
        qDebug() << "getPrescriptionsByDoctor error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject prescription;
        prescription["id"] = query.value("id").toInt();
        prescription["record_id"] = query.value("record_id").toInt();
        prescription["patient_username"] = query.value("patient_username").toString();
        prescription["prescription_date"] = query.value("prescription_date").toString();
        prescription["total_amount"] = query.value("total_amount").toDouble();
        prescription["status"] = query.value("status").toString();
        prescription["notes"] = query.value("notes").toString();
        prescription["patient_name"] = query.value("patient_name").toString();
        prescription["patient_age"] = query.value("patient_age").toInt();
        prescriptions.append(prescription);
    }
    return true;
}

bool DBManager::getPrescriptionDetails(int prescriptionId, QJsonObject& prescription) {
    // 首先获取处方基本信息
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT p.id, p.record_id, p.patient_username, p.doctor_username,
               p.prescription_date, p.total_amount, p.status, p.notes,
               pt.name as patient_name, d.name as doctor_name, d.title as doctor_title
        FROM prescriptions p
        LEFT JOIN patients pt ON p.patient_username = pt.username
        LEFT JOIN doctors d ON p.doctor_username = d.username
        WHERE p.id = :prescription_id
    )");
    query.bindValue(":prescription_id", prescriptionId);
    
    if (!query.exec()) {
        qDebug() << "getPrescriptionDetails error:" << query.lastError().text();
        return false;
    }
    
    if (!query.next()) {
        qDebug() << "Prescription not found:" << prescriptionId;
        return false;
    }
    
    // 填充处方基本信息
    prescription["id"] = query.value("id").toInt();
    prescription["record_id"] = query.value("record_id").toInt();
    prescription["patient_username"] = query.value("patient_username").toString();
    prescription["doctor_username"] = query.value("doctor_username").toString();
    prescription["prescription_date"] = query.value("prescription_date").toString();
    prescription["total_amount"] = query.value("total_amount").toDouble();
    prescription["status"] = query.value("status").toString();
    prescription["notes"] = query.value("notes").toString();
    prescription["patient_name"] = query.value("patient_name").toString();
    prescription["doctor_name"] = query.value("doctor_name").toString();
    prescription["doctor_title"] = query.value("doctor_title").toString();
    
    // 获取处方项目详情
    QSqlQuery itemQuery(m_db);
    itemQuery.prepare(R"(
        SELECT pi.id, pi.medication_id, pi.quantity, pi.dosage, pi.frequency,
               pi.duration, pi.instructions, pi.unit_price, pi.total_price,
               m.name as medication_name, m.category, m.unit
        FROM prescription_items pi
        LEFT JOIN medications m ON pi.medication_id = m.id
        WHERE pi.prescription_id = :prescription_id
    )");
    itemQuery.bindValue(":prescription_id", prescriptionId);
    
    if (!itemQuery.exec()) {
        qDebug() << "getPrescriptionItems error:" << itemQuery.lastError().text();
        return false;
    }
    
    QJsonArray items;
    while (itemQuery.next()) {
        QJsonObject item;
        item["id"] = itemQuery.value("id").toInt();
        item["medication_id"] = itemQuery.value("medication_id").toInt();
        item["quantity"] = itemQuery.value("quantity").toInt();
        item["dosage"] = itemQuery.value("dosage").toString();
        item["frequency"] = itemQuery.value("frequency").toString();
        item["duration"] = itemQuery.value("duration").toString();
        item["instructions"] = itemQuery.value("instructions").toString();
        item["unit_price"] = itemQuery.value("unit_price").toDouble();
        item["total_price"] = itemQuery.value("total_price").toDouble();
        item["medication_name"] = itemQuery.value("medication_name").toString();
        item["category"] = itemQuery.value("category").toString();
        item["unit"] = itemQuery.value("unit").toString();
        items.append(item);
    }
    
    prescription["items"] = items;
    return true;
}

// 药品管理实现 - 提供基础实现
bool DBManager::addMedication(const QJsonObject& medicationData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO medications (
            name, generic_name, category, manufacturer, specification,
            unit, price, stock_quantity, description, precautions,
            side_effects, contraindications
        ) VALUES (
            :name, :generic_name, :category, :manufacturer, :specification,
            :unit, :price, :stock_quantity, :description, :precautions,
            :side_effects, :contraindications
        )
    )");
    
    query.bindValue(":name", medicationData["name"].toString());
    query.bindValue(":generic_name", medicationData["generic_name"].toString());
    query.bindValue(":category", medicationData["category"].toString());
    query.bindValue(":manufacturer", medicationData["manufacturer"].toString());
    query.bindValue(":specification", medicationData["specification"].toString());
    query.bindValue(":unit", medicationData["unit"].toString());
    query.bindValue(":price", medicationData["price"].toDouble());
    query.bindValue(":stock_quantity", medicationData["stock_quantity"].toInt());
    query.bindValue(":description", medicationData["description"].toString());
    query.bindValue(":precautions", medicationData["precautions"].toString());
    query.bindValue(":side_effects", medicationData["side_effects"].toString());
    query.bindValue(":contraindications", medicationData["contraindications"].toString());
    
    if (!query.exec()) {
        qDebug() << "addMedication error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getMedications(QJsonArray& medications) {
    QSqlQuery query(m_db);
    if (!query.exec("SELECT id, name, generic_name, category, manufacturer, specification, unit, price, stock_quantity, description, precautions FROM medications")) {
        qDebug() << "getMedications error:" << query.lastError().text();
        return false;
    }
    while (query.next()) {
        QJsonObject medication;
        medication["id"] = query.value("id").toInt();
        medication["name"] = query.value("name").toString();
        medication["generic_name"] = query.value("generic_name").toString();
        medication["category"] = query.value("category").toString();
        medication["manufacturer"] = query.value("manufacturer").toString();
        medication["specification"] = query.value("specification").toString();
        medication["unit"] = query.value("unit").toString();
        medication["price"] = query.value("price").toDouble();
        medication["stock_quantity"] = query.value("stock_quantity").toInt();
        medication["description"] = query.value("description").toString();
        medication["precautions"] = query.value("precautions").toString();
        medications.append(medication);
    }
    return true;
}

bool DBManager::searchMedications(const QString& keyword, QJsonArray& medications) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, name, generic_name, category, manufacturer, specification,
               unit, price, stock_quantity, description, precautions
        FROM medications
        WHERE name = :keyword OR generic_name = :keyword
              OR category LIKE :like_keyword OR manufacturer LIKE :like_keyword
        ORDER BY name
    )");
    query.bindValue(":keyword", keyword);
    query.bindValue(":like_keyword", "%" + keyword + "%");
    
    if (!query.exec()) {
        qDebug() << "searchMedications error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject medication;
        medication["id"] = query.value("id").toInt();
        medication["name"] = query.value("name").toString();
        medication["generic_name"] = query.value("generic_name").toString();
        medication["category"] = query.value("category").toString();
        medication["manufacturer"] = query.value("manufacturer").toString();
        medication["specification"] = query.value("specification").toString();
        medication["unit"] = query.value("unit").toString();
        medication["price"] = query.value("price").toDouble();
        medication["stock_quantity"] = query.value("stock_quantity").toInt();
    medication["description"] = query.value("description").toString();
    medication["precautions"] = query.value("precautions").toString();
        medications.append(medication);
    }
    return true;
}

// 统计查询实现 - 提供基础实现
bool DBManager::getDoctorStatistics(const QString& doctorUsername, QJsonObject& statistics) {
    // 获取医生预约统计
    QSqlQuery appointmentQuery(m_db);
    appointmentQuery.prepare(R"(
        SELECT 
            COUNT(*) as total_appointments,
            SUM(CASE WHEN status = 'pending' THEN 1 ELSE 0 END) as pending_appointments,
            SUM(CASE WHEN status = 'confirmed' THEN 1 ELSE 0 END) as confirmed_appointments,
            SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END) as completed_appointments,
            SUM(CASE WHEN status = 'cancelled' THEN 1 ELSE 0 END) as cancelled_appointments,
            SUM(COALESCE(fee, 0)) as total_fees
        FROM appointments
        WHERE doctor_username = :doctor_username
    )");
    appointmentQuery.bindValue(":doctor_username", doctorUsername);
    
    if (!appointmentQuery.exec()) {
        qDebug() << "getDoctorStatistics appointments error:" << appointmentQuery.lastError().text();
        return false;
    }
    
    if (appointmentQuery.next()) {
        statistics["total_appointments"] = appointmentQuery.value("total_appointments").toInt();
        statistics["pending_appointments"] = appointmentQuery.value("pending_appointments").toInt();
        statistics["confirmed_appointments"] = appointmentQuery.value("confirmed_appointments").toInt();
        statistics["completed_appointments"] = appointmentQuery.value("completed_appointments").toInt();
        statistics["cancelled_appointments"] = appointmentQuery.value("cancelled_appointments").toInt();
        statistics["total_fees"] = appointmentQuery.value("total_fees").toDouble();
    }
    
    // 获取医生病例统计
    QSqlQuery recordQuery(m_db);
    recordQuery.prepare("SELECT COUNT(*) as total_records FROM medical_records WHERE doctor_username = :doctor_username");
    recordQuery.bindValue(":doctor_username", doctorUsername);
    
    if (recordQuery.exec() && recordQuery.next()) {
        statistics["total_medical_records"] = recordQuery.value("total_records").toInt();
    }
    
    // 获取医生处方统计
    QSqlQuery prescriptionQuery(m_db);
    prescriptionQuery.prepare(R"(
        SELECT 
            COUNT(*) as total_prescriptions,
            SUM(COALESCE(total_amount, 0)) as total_prescription_amount
        FROM prescriptions
        WHERE doctor_username = :doctor_username
    )");
    prescriptionQuery.bindValue(":doctor_username", doctorUsername);
    
    if (prescriptionQuery.exec() && prescriptionQuery.next()) {
        statistics["total_prescriptions"] = prescriptionQuery.value("total_prescriptions").toInt();
        statistics["total_prescription_amount"] = prescriptionQuery.value("total_prescription_amount").toDouble();
    }
    
    return true;
}

bool DBManager::getPatientStatistics(const QString& patientUsername, QJsonObject& statistics) {
    // 获取患者预约统计
    QSqlQuery appointmentQuery(m_db);
    appointmentQuery.prepare(R"(
        SELECT 
            COUNT(*) as total_appointments,
            SUM(CASE WHEN status = 'pending' THEN 1 ELSE 0 END) as pending_appointments,
            SUM(CASE WHEN status = 'confirmed' THEN 1 ELSE 0 END) as confirmed_appointments,
            SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END) as completed_appointments,
            SUM(CASE WHEN status = 'cancelled' THEN 1 ELSE 0 END) as cancelled_appointments,
            SUM(COALESCE(fee, 0)) as total_fees
        FROM appointments
        WHERE patient_username = :patient_username
    )");
    appointmentQuery.bindValue(":patient_username", patientUsername);
    
    if (!appointmentQuery.exec()) {
        qDebug() << "getPatientStatistics appointments error:" << appointmentQuery.lastError().text();
        return false;
    }
    
    if (appointmentQuery.next()) {
        statistics["total_appointments"] = appointmentQuery.value("total_appointments").toInt();
        statistics["pending_appointments"] = appointmentQuery.value("pending_appointments").toInt();
        statistics["confirmed_appointments"] = appointmentQuery.value("confirmed_appointments").toInt();
        statistics["completed_appointments"] = appointmentQuery.value("completed_appointments").toInt();
        statistics["cancelled_appointments"] = appointmentQuery.value("cancelled_appointments").toInt();
        statistics["total_fees"] = appointmentQuery.value("total_fees").toDouble();
    }
    
    // 获取患者病例统计
    QSqlQuery recordQuery(m_db);
    recordQuery.prepare("SELECT COUNT(*) as total_records FROM medical_records WHERE patient_username = :patient_username");
    recordQuery.bindValue(":patient_username", patientUsername);
    
    if (recordQuery.exec() && recordQuery.next()) {
        statistics["total_medical_records"] = recordQuery.value("total_records").toInt();
    }
    
    // 获取患者处方统计
    QSqlQuery prescriptionQuery(m_db);
    prescriptionQuery.prepare(R"(
        SELECT 
            COUNT(*) as total_prescriptions,
            SUM(COALESCE(total_amount, 0)) as total_prescription_amount
        FROM prescriptions
        WHERE patient_username = :patient_username
    )");
    prescriptionQuery.bindValue(":patient_username", patientUsername);
    
    if (prescriptionQuery.exec() && prescriptionQuery.next()) {
        statistics["total_prescriptions"] = prescriptionQuery.value("total_prescriptions").toInt();
        statistics["total_prescription_amount"] = prescriptionQuery.value("total_prescription_amount").toDouble();
    }
    
    // 获取最近就诊医生信息
    QSqlQuery recentDoctorQuery(m_db);
    recentDoctorQuery.prepare(R"(
        SELECT DISTINCT a.doctor_username, d.name as doctor_name, d.department
        FROM appointments a
        LEFT JOIN doctors d ON a.doctor_username = d.username
        WHERE a.patient_username = :patient_username AND a.status = 'completed'
        ORDER BY a.appointment_date DESC
        LIMIT 5
    )");
    recentDoctorQuery.bindValue(":patient_username", patientUsername);
    
    if (recentDoctorQuery.exec()) {
        QJsonArray recentDoctors;
        while (recentDoctorQuery.next()) {
            QJsonObject doctor;
            doctor["username"] = recentDoctorQuery.value("doctor_username").toString();
            doctor["name"] = recentDoctorQuery.value("doctor_name").toString();
            doctor["department"] = recentDoctorQuery.value("department").toString();
            recentDoctors.append(doctor);
        }
        statistics["recent_doctors"] = recentDoctors;
    }
    
    return true;
}

// 获取用户角色
QString DBManager::getUserRole(const QString& username) {
    QSqlQuery query(m_db);
    query.prepare("SELECT role FROM users WHERE username = :username");
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        return query.value("role").toString();
    }
    
    qDebug() << "getUserRole error:" << query.lastError().text();
    return QString();
}

// 获取所有医生列表
bool DBManager::getAllDoctors(QJsonArray& doctors) {
    QSqlQuery query(m_db);
    if (!query.exec("SELECT username, name, department, title, specialization, consultation_fee FROM doctors")) {
        qDebug() << "getAllDoctors error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject doctor;
        doctor["username"] = query.value("username").toString();
        doctor["name"] = query.value("name").toString();
        doctor["department"] = query.value("department").toString();
        doctor["title"] = query.value("title").toString();
        doctor["specialization"] = query.value("specialization").toString();
        doctor["consultation_fee"] = query.value("consultation_fee").toDouble();
        doctors.append(doctor);
    }
    return true;
}

// 按科室获取医生列表
bool DBManager::getDoctorsByDepartment(const QString& department, QJsonArray& doctors) {
    QSqlQuery query(m_db);
    query.prepare("SELECT username, name, department, title, specialization, consultation_fee FROM doctors WHERE department = :department");
    query.bindValue(":department", department);
    
    if (!query.exec()) {
        qDebug() << "getDoctorsByDepartment error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject doctor;
        doctor["username"] = query.value("username").toString();
        doctor["name"] = query.value("name").toString();
        doctor["department"] = query.value("department").toString();
        doctor["title"] = query.value("title").toString();
        doctor["specialization"] = query.value("specialization").toString();
        doctor["consultation_fee"] = query.value("consultation_fee").toDouble();
        doctors.append(doctor);
    }
    return true;
}

// 清理不一致的数据
bool DBManager::cleanupInconsistentData() {
    // 此处可以添加数据清理逻辑，例如：
    // 1. 删除在 'users' 表中没有对应用户的 'doctors' 或 'patients' 记录
    // 2. 删除引用了不存在的用户的预约
    // 3. 确保外键约束的完整性
    // 目前仅为占位符
    QSqlQuery query(m_db);

    // 清理 doctors 表中不存在于 users 表的记录
    QString cleanupDoctorsSql = "DELETE FROM doctors WHERE username NOT IN (SELECT username FROM users)";
    if (!query.exec(cleanupDoctorsSql)) {
        qDebug() << "Failed to cleanup doctors table:" << query.lastError().text();
        return false;
    }

    // 清理 patients 表中不存在于 users 表的记录
    QString cleanupPatientsSql = "DELETE FROM patients WHERE username NOT IN (SELECT username FROM users)";
    if (!query.exec(cleanupPatientsSql)) {
        qDebug() << "Failed to cleanup patients table:" << query.lastError().text();
        return false;
    }

    // 可以根据需要添加更多清理规则...

    qDebug() << "Inconsistent data cleanup completed successfully.";
    return true;
}

// 插入示例医生数据
void DBManager::insertSampleDoctors() {
    // 检查是否已经有医生数据
    QSqlQuery checkQuery(m_db);
    if (checkQuery.exec("SELECT COUNT(*) FROM doctors") && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        return; // 如果已有数据，不再插入
    }
    
    // 插入示例医生数据
    QJsonArray doctors;
    
    QJsonObject doctor1;
    doctor1["username"] = "doctor1";
    doctor1["name"] = "张明";
    doctor1["department"] = "内科";
    doctor1["title"] = "主任医师";
    doctor1["specialization"] = "心血管疾病";
    doctor1["consultation_fee"] = 50.0;
    doctors.append(doctor1);
    
    QJsonObject doctor2;
    doctor2["username"] = "doctor2";
    doctor2["name"] = "李华";
    doctor2["department"] = "外科";
    doctor2["title"] = "副主任医师";
    doctor2["specialization"] = "骨科手术";
    doctor2["consultation_fee"] = 60.0;
    doctors.append(doctor2);
    
    for (const auto& doctorValue : doctors) {
        QJsonObject doctor = doctorValue.toObject();
        QString username = doctor["username"].toString();
        
        // 首先添加用户
        QSqlQuery userQuery(m_db);
        userQuery.prepare("INSERT OR IGNORE INTO users (username, password, role) VALUES (:username, :password, :role)");
        userQuery.bindValue(":username", username);
        userQuery.bindValue(":password", "123456"); // 默认密码
        userQuery.bindValue(":role", "doctor");
        
        if (!userQuery.exec()) {
            qDebug() << "Insert sample user error:" << userQuery.lastError().text();
            continue;
        }
        
        // 然后添加医生信息
        QSqlQuery updateQuery(m_db);
        updateQuery.prepare(R"(
            INSERT OR REPLACE INTO doctors 
            (username, name, department, title, specialization, consultation_fee) 
            VALUES (?, ?, ?, ?, ?, ?)
        )");
        updateQuery.addBindValue(username);
        updateQuery.addBindValue(doctor["name"].toString());
        updateQuery.addBindValue(doctor["department"].toString());
        updateQuery.addBindValue(doctor["title"].toString());
        updateQuery.addBindValue(doctor["specialization"].toString());
        updateQuery.addBindValue(doctor["consultation_fee"].toDouble());
        
        if (!updateQuery.exec()) {
            qDebug() << "Insert sample doctor error:" << updateQuery.lastError().text();
        }
    }
}

// 插入示例药品数据
void DBManager::insertSampleMedications() {
    // 检查是否已经有药品数据
    QSqlQuery checkQuery(m_db);
    if (checkQuery.exec("SELECT COUNT(*) FROM medications") && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        return; // 如果已有数据，不再插入
    }
    
    // 插入示例药品数据
    QJsonArray medications;
    
    QJsonObject med1;
    med1["name"] = "阿司匹林肠溶片";
    med1["generic_name"] = "Aspirin Enteric-coated Tablets";
    med1["category"] = "解热镇痛";
    med1["manufacturer"] = "拜耳";
    med1["specification"] = "100mg*30片";
    med1["unit"] = "盒";
    med1["price"] = 15.50;
    med1["stock_quantity"] = 1000;
    medications.append(med1);
    
    QJsonObject med2;
    med2["name"] = "布洛芬缓释胶囊";
    med2["generic_name"] = "Ibuprofen Sustained-release Capsules";
    med2["category"] = "解热镇痛";
    med2["manufacturer"] = "芬必得";
    med2["specification"] = "0.3g*20粒";
    med2["unit"] = "盒";
    med2["price"] = 22.00;
    med2["stock_quantity"] = 800;
    medications.append(med2);
    
    for (const auto& medValue : medications) {
        addMedication(medValue.toObject());
    }
}
