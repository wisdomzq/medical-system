#include "database.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>
#include <QCoreApplication>

DBManager::DBManager(const QString& path) {
    // 检查可用的SQL驱动
    qDebug() << "可用的SQL驱动:" << QSqlDatabase::drivers();
    
    // 使用唯一的连接名称，避免与TcpServer冲突
    static int connectionId = 0;
    QString connectionName = QString("main_db_connection_%1").arg(++connectionId);
    
    m_db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    m_db.setDatabaseName(path);
    
    qDebug() << "数据库驱动名称:" << m_db.driverName();
    qDebug() << "数据库文件路径:" << path;

    if (!m_db.open()) {
        qDebug() << "Error: connection with database fail" << m_db.lastError().text();
    } else {
        qDebug() << "Database: connection ok";
    }
    initDatabase();
}

DBManager::~DBManager() {
    const QString connectionName = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    // 释放该连接的最后一个句柄
    m_db = QSqlDatabase();

    // 如果有事件循环，延后到本轮事件循环结束再移除连接，
    // 可避免析构顺序导致的 "still in use" 警告。
    if (QCoreApplication::instance()) {
        QTimer::singleShot(0, [connectionName]() {
            QSqlDatabase::removeDatabase(connectionName);
        });
    } else {
        // 无事件循环时，直接移除
        QSqlDatabase::removeDatabase(connectionName);
    }
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
    createHospitalizationsTable(); // 添加住院表
    createAttendanceTable();
    createLeaveRequestsTable();
    createChatMessagesTable();
    
    // 不插入示例数据，使用现有数据库中的数据
}

void DBManager::createChatMessagesTable() {
    if (!m_db.tables().contains(QStringLiteral("chat_messages"))) {
        QSqlQuery q(m_db);
        const char *sql = R"(
            CREATE TABLE chat_messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                doctor_username  TEXT NOT NULL,
                patient_username TEXT NOT NULL,
                message_id       TEXT NOT NULL UNIQUE,
                sender_username  TEXT NOT NULL,
                message_type     TEXT NOT NULL CHECK(message_type IN ('text','image','file')),
                text_content     TEXT,
                file_metadata    TEXT,
                created_at       DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (doctor_username)  REFERENCES users(username),
                FOREIGN KEY (patient_username) REFERENCES users(username),
                FOREIGN KEY (sender_username)  REFERENCES users(username)
            )
        )";
        if (!q.exec(sql)) {
            qDebug() << "创建chat_messages表失败:" << q.lastError().text();
        } else {
            QSqlQuery idx(m_db);
            idx.exec("CREATE INDEX IF NOT EXISTS idx_chat_pair ON chat_messages(doctor_username, patient_username)");
            idx.exec("CREATE INDEX IF NOT EXISTS idx_chat_pair_id ON chat_messages(doctor_username, patient_username, id DESC)");
            idx.exec("CREATE INDEX IF NOT EXISTS idx_chat_sender ON chat_messages(sender_username, id DESC)");
        }
    }
}

bool DBManager::addChatMessage(const QJsonObject &msg, int &insertedId, QString &errorMessage) {
    // 期望字段：doctor_username, patient_username, message_id, sender_username, message_type, text_content(可空), file_metadata(可空)
    QSqlQuery q(m_db);
    q.prepare(R"(
        INSERT INTO chat_messages (
            doctor_username, patient_username, message_id, sender_username,
            message_type, text_content, file_metadata
        ) VALUES (:d, :p, :mid, :s, :t, :text, :file)
    )");
    q.bindValue(":d", msg.value("doctor_username").toString());
    q.bindValue(":p", msg.value("patient_username").toString());
    q.bindValue(":mid", msg.value("message_id").toString());
    q.bindValue(":s", msg.value("sender_username").toString());
    q.bindValue(":t", msg.value("message_type").toString());
    q.bindValue(":text", msg.value("text_content").toString());
    // file_metadata 若为对象，序列化为字符串
    if (msg.contains("file_metadata") && msg.value("file_metadata").isObject()) {
        q.bindValue(":file", QString::fromUtf8(QJsonDocument(msg.value("file_metadata").toObject()).toJson(QJsonDocument::Compact)));
    } else if (msg.contains("file_metadata") && msg.value("file_metadata").isString()) {
        q.bindValue(":file", msg.value("file_metadata").toString());
    } else {
        q.bindValue(":file", QVariant(QVariant::String));
    }
    if (!q.exec()) {
        errorMessage = q.lastError().text();
        return false;
    }
    insertedId = q.lastInsertId().toInt();
    return true;
}

bool DBManager::getChatHistory(const QString &doctorUsername, const QString &patientUsername,
                               qint64 beforeId, int limit, QJsonArray &out) {
    QSqlQuery q(m_db);
    QString sql = R"(
        SELECT id, doctor_username, patient_username, message_id, sender_username,
               message_type, text_content, file_metadata, created_at
        FROM chat_messages
        WHERE doctor_username = :d AND patient_username = :p
    )";
    if (beforeId > 0) sql += " AND id < :before";
    sql += " ORDER BY id DESC LIMIT :limit";
    q.prepare(sql);
    q.bindValue(":d", doctorUsername);
    q.bindValue(":p", patientUsername);
    if (beforeId > 0) q.bindValue(":before", beforeId);
    q.bindValue(":limit", qMax(1, limit));
    if (!q.exec()) { qDebug() << "getChatHistory error:" << q.lastError().text(); return false; }
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value("id").toInt();
        o["doctor_username"] = q.value("doctor_username").toString();
        o["patient_username"] = q.value("patient_username").toString();
        o["message_id"] = q.value("message_id").toString();
        o["sender_username"] = q.value("sender_username").toString();
        o["message_type"] = q.value("message_type").toString();
        o["text_content"] = q.value("text_content").toString();
        const QString fm = q.value("file_metadata").toString();
        if (!fm.isEmpty()) {
            QJsonParseError err; auto doc = QJsonDocument::fromJson(fm.toUtf8(), &err);
            o["file_metadata"] = (err.error==QJsonParseError::NoError && doc.isObject()) ? doc.object() : QJsonObject();
        } else o["file_metadata"] = QJsonValue();
        o["created_at"] = q.value("created_at").toString();
        out.append(o);
    }
    return true;
}

bool DBManager::getMessagesSinceForUser(const QString &username, qint64 cursor, int limit, QJsonArray &out) {
    // 返回与该用户相关（作为医生、患者、或发送者）且 id>cursor 的消息
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT id, doctor_username, patient_username, message_id, sender_username,
               message_type, text_content, file_metadata, created_at
        FROM chat_messages
        WHERE id > :cursor AND (
            doctor_username = :u OR patient_username = :u OR sender_username = :u
        )
        ORDER BY id ASC
        LIMIT :limit
    )");
    q.bindValue(":cursor", cursor);
    q.bindValue(":u", username);
    q.bindValue(":limit", qMax(1, limit));
    if (!q.exec()) { qDebug() << "getMessagesSinceForUser error:" << q.lastError().text(); return false; }
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value("id").toInt();
        o["doctor_username"] = q.value("doctor_username").toString();
        o["patient_username"] = q.value("patient_username").toString();
        o["message_id"] = q.value("message_id").toString();
        o["sender_username"] = q.value("sender_username").toString();
        o["message_type"] = q.value("message_type").toString();
        o["text_content"] = q.value("text_content").toString();
        const QString fm = q.value("file_metadata").toString();
        if (!fm.isEmpty()) {
            QJsonParseError err; auto doc = QJsonDocument::fromJson(fm.toUtf8(), &err);
            o["file_metadata"] = (err.error==QJsonParseError::NoError && doc.isObject()) ? doc.object() : QJsonObject();
        } else o["file_metadata"] = QJsonValue();
        o["created_at"] = q.value("created_at").toString();
        out.append(o);
    }
    return true;
}

bool DBManager::getRecentContactsForUser(const QString &username, int limit, QJsonArray &out) {
    // 查找最近出现于该用户相关消息中的对端用户名（医生或患者），按最近消息时间倒序，去重
    // 通过 UNION 取医生视角与患者视角，再按 max(id) 排序
    QSqlQuery q(m_db);
    q.prepare(R"(
        SELECT peer, MAX(id) AS last_id
        FROM (
            SELECT CASE WHEN sender_username = :u THEN
                        CASE WHEN doctor_username = :u THEN patient_username ELSE doctor_username END
                    ELSE
                        CASE WHEN doctor_username = :u THEN patient_username ELSE doctor_username END
                END AS peer,
                id
            FROM chat_messages
            WHERE doctor_username = :u OR patient_username = :u OR sender_username = :u
        ) t
        GROUP BY peer
        ORDER BY last_id DESC
        LIMIT :limit
    )");
    q.bindValue(":u", username);
    q.bindValue(":limit", qMax(1, limit));
    if (!q.exec()) { qDebug() << "getRecentContactsForUser error:" << q.lastError().text(); return false; }
    while (q.next()) {
        QJsonObject o; o["username"] = q.value(0).toString(); o["last_id"] = q.value(1).toInt();
        out.append(o);
    }
    return true;
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
                photo BLOB,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建doctors表失败:" << query.lastError().text();
        }
    }
    // 表已存在时，确保新增列 photo 存在（SQLite 迁移）
    {
        QSqlQuery info(m_db);
        if (info.exec("PRAGMA table_info(doctors)")) {
            bool hasPhoto = false;
            while (info.next()) {
                if (info.value(1).toString().compare("photo", Qt::CaseInsensitive) == 0) {
                    hasPhoto = true; break;
                }
            }
            if (!hasPhoto) {
                QSqlQuery alter(m_db);
                if (!alter.exec("ALTER TABLE doctors ADD COLUMN photo BLOB")) {
                    qDebug() << "为doctors表添加photo列失败:" << alter.lastError().text();
                }
            }
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

void DBManager::createHospitalizationsTable() {
    if (!m_db.tables().contains(QStringLiteral("hospitalizations"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE hospitalizations (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                patient_username TEXT NOT NULL,
                doctor_username TEXT NOT NULL,
                admission_date DATE NOT NULL,
                discharge_date DATE,
                ward TEXT,
                bed_number TEXT,
                diagnosis TEXT,
                treatment_plan TEXT,
                daily_cost DECIMAL(10,2) DEFAULT 0.00,
                total_cost DECIMAL(10,2) DEFAULT 0.00,
                status TEXT DEFAULT 'admitted' CHECK(status IN ('admitted', 'discharged', 'transferred')),
                notes TEXT,
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

void DBManager::createAttendanceTable() {
    if (!m_db.tables().contains(QStringLiteral("attendance"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE attendance (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                doctor_username TEXT NOT NULL,
                checkin_date DATE NOT NULL,
                checkin_time TIME NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建attendance表失败:" << query.lastError().text();
        } else {
            QSqlQuery idx(m_db);
            idx.exec("CREATE INDEX IF NOT EXISTS idx_attendance_doctor_date ON attendance(doctor_username, checkin_date)");
        }
    }
}

void DBManager::createLeaveRequestsTable() {
    if (!m_db.tables().contains(QStringLiteral("leave_requests"))) {
        QSqlQuery query(m_db);
        QString sql = R"(
            CREATE TABLE leave_requests (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                doctor_username TEXT NOT NULL,
                leave_date DATE NOT NULL,
                reason TEXT,
                status TEXT NOT NULL DEFAULT 'active' CHECK(status IN ('active','cancelled','approved','rejected')),
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (doctor_username) REFERENCES users(username)
            )
        )";
        if (!query.exec(sql)) {
            qDebug() << "创建leave_requests表失败:" << query.lastError().text();
        } else {
            QSqlQuery idx(m_db);
            idx.exec("CREATE INDEX IF NOT EXISTS idx_leave_doctor_status ON leave_requests(doctor_username, status)");
        }
    }
}

bool DBManager::createAttendanceRecord(const QJsonObject &data) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO attendance (doctor_username, checkin_date, checkin_time)
                VALUES (:u, :d, :t))");
    q.bindValue(":u", data.value("doctor_username").toString());
    q.bindValue(":d", data.value("checkin_date").toString());
    q.bindValue(":t", data.value("checkin_time").toString());
    if (!q.exec()) { qDebug() << "createAttendanceRecord error:" << q.lastError().text(); return false; }
    return true;
}

bool DBManager::getAttendanceByDoctor(const QString &doctorUsername, QJsonArray &records, int limit) {
    QSqlQuery q(m_db);
    q.prepare(QString(R"(
        SELECT id, doctor_username, checkin_date, checkin_time, created_at
        FROM attendance
        WHERE doctor_username = :u
        ORDER BY checkin_date DESC, checkin_time DESC, id DESC
        LIMIT %1
    )").arg(qMax(1, limit)));
    q.bindValue(":u", doctorUsername);
    if (!q.exec()) { qDebug() << "getAttendanceByDoctor error:" << q.lastError().text(); return false; }
    while (q.next()) {
        QJsonObject o;
        o["id"] = q.value("id").toInt();
        o["doctor_username"] = q.value("doctor_username").toString();
        o["checkin_date"] = q.value("checkin_date").toString();
        o["checkin_time"] = q.value("checkin_time").toString();
        o["created_at"] = q.value("created_at").toString();
        records.append(o);
    }
    return true;
}

bool DBManager::createLeaveRequest(const QJsonObject &data) {
    QSqlQuery q(m_db);
    q.prepare(R"(INSERT INTO leave_requests (doctor_username, leave_date, reason, status)
                VALUES (:u, :d, :r, 'active'))");
    q.bindValue(":u", data.value("doctor_username").toString());
    q.bindValue(":d", data.value("leave_date").toString());
    q.bindValue(":r", data.value("reason").toString());
    if (!q.exec()) { qDebug() << "createLeaveRequest error:" << q.lastError().text(); return false; }
    return true;
}

bool DBManager::getActiveLeavesByDoctor(const QString &doctorUsername, QJsonArray &leaves) {
    QSqlQuery q(m_db);
    q.prepare(R"(SELECT id, doctor_username, leave_date, reason, status, created_at FROM leave_requests
                 WHERE doctor_username=:u AND status='active' ORDER BY created_at DESC)");
    q.bindValue(":u", doctorUsername);
    if (!q.exec()) { qDebug() << "getActiveLeavesByDoctor error:" << q.lastError().text(); return false; }
    while (q.next()) {
        QJsonObject o; o["id"] = q.value("id").toInt(); o["doctor_username"] = q.value("doctor_username").toString();
        o["leave_date"] = q.value("leave_date").toString(); o["reason"] = q.value("reason").toString();
        o["status"] = q.value("status").toString(); o["created_at"] = q.value("created_at").toString();
        leaves.append(o);
    }
    return true;
}

bool DBManager::cancelLeaveById(int leaveId) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE leave_requests SET status='cancelled', updated_at=CURRENT_TIMESTAMP WHERE id=:id AND status='active'");
    q.bindValue(":id", leaveId);
    if (!q.exec()) { qDebug() << "cancelLeaveById error:" << q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

bool DBManager::cancelActiveLeaveForDoctor(const QString &doctorUsername) {
    QSqlQuery q(m_db);
    q.prepare(R"(UPDATE leave_requests
                 SET status='cancelled', updated_at=CURRENT_TIMESTAMP
                 WHERE id IN (
                    SELECT id FROM leave_requests WHERE doctor_username=:u AND status='active' ORDER BY created_at DESC LIMIT 1
                 ))");
    q.bindValue(":u", doctorUsername);
    if (!q.exec()) { qDebug() << "cancelActiveLeaveForDoctor error:" << q.lastError().text(); return false; }
    return q.numRowsAffected() > 0;
}

// 移除旧版住院接口的重复实现，统一使用下方新版实现

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
    qDebug() << "添加用户到users表:" << username << "role:" << role;
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, role) VALUES (:username, :password, :role)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.bindValue(":role", role);
    if (!query.exec()) {
        qDebug() << "addUser error:" << query.lastError().text();
        return false;
    }
    qDebug() << "用户成功添加到users表:" << username;
    return true;
}

bool DBManager::getDoctorInfo(const QString& username, QJsonObject& doctorInfo) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT name, department, phone, email, work_number, title, 
               specialization, consultation_fee, max_patients_per_day, photo
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
        QByteArray photo = query.value("photo").toByteArray();
        if (!photo.isEmpty()) {
            doctorInfo["photo"] = QString::fromUtf8(photo.toBase64());
        }
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
    // 动态构造 SQL，只有在传入 photo 时才更新照片，避免无意清空
    QStringList setClauses;
    setClauses << "name = :name"
               << "department = :department"
               << "phone = :phone"
               << "email = :email"
               << "work_number = :work_number"
               << "title = :title"
               << "specialization = :specialization"
               << "consultation_fee = :consultation_fee"
               << "max_patients_per_day = :max_patients_per_day";
    bool withPhoto = data.contains("photo");
    if (withPhoto) {
        setClauses << "photo = :photo";
    }
    QString sql = QString("UPDATE doctors SET %1, updated_at = CURRENT_TIMESTAMP WHERE username = :username")
                      .arg(setClauses.join(", "));
    QSqlQuery query(m_db);
    query.prepare(sql);

    query.bindValue(":name", data.value("name").toString());
    query.bindValue(":department", data.value("department").toString());
    query.bindValue(":phone", data.value("phone").toString());
    query.bindValue(":email", data.value("email").toString());
    query.bindValue(":work_number", data.value("work_number").toString());
    query.bindValue(":title", data.value("title").toString());
    query.bindValue(":specialization", data.value("specialization").toString());
    query.bindValue(":consultation_fee", data.value("consultation_fee").toDouble());
    query.bindValue(":max_patients_per_day", data.value("max_patients_per_day").toInt());
    if (withPhoto) {
        QByteArray ba = QByteArray::fromBase64(data.value("photo").toString().toUtf8());
        query.bindValue(":photo", ba);
    }
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
    // 开启数据库事务
    if (!m_db.transaction()) {
        qDebug() << "Failed to start transaction.";
        return false;
    }

    // 1. 插入到 users 表
    if (!addUser(name, password, "doctor")) {
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
    
    return true;
}

// 重写 registerPatient，使用事务确保原子性，并一次性插入所有数据。
bool DBManager::registerPatient(const QString& name, const QString& password, int age, const QString& phone, const QString& address) {
    qDebug() << "开始注册患者:" << name << "age:" << age << "phone:" << phone << "address:" << address;
    
    if (!m_db.transaction()) {
        qDebug() << "Failed to start transaction.";
        return false;
    }

    // 1. 插入到 users 表
    if (!addUser(name, password, "patient")) {
        qDebug() << "Failed to add user to users table";
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
    
    qDebug() << "患者注册成功:" << name;
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
    QString doctorUsername = appointmentData["doctor_username"].toString();
    QString appointmentDate = appointmentData["appointment_date"].toString();
    QString appointmentTime = appointmentData["appointment_time"].toString();
    
    // 尝试严格的验证流程
    bool strictValidationSuccess = true;
    QString doctorDepartment;
    double consultationFee = 0.0;
    
    // 1. 验证医生是否存在并获取医生信息
    QSqlQuery doctorQuery(m_db);
    doctorQuery.prepare(R"(
        SELECT d.name, d.department, d.consultation_fee, d.title, d.specialization
        FROM doctors d 
        WHERE d.username = :username
    )");
    doctorQuery.bindValue(":username", doctorUsername);
    
    if (!doctorQuery.exec() || !doctorQuery.next()) {
        qDebug() << "Doctor not found in doctors table, trying fallback:" << doctorQuery.lastError().text();
        strictValidationSuccess = false;
    } else {
        doctorDepartment = doctorQuery.value("department").toString();
        consultationFee = doctorQuery.value("consultation_fee").toDouble();
    }
    
    // 2. 如果医生存在，验证排班
    if (strictValidationSuccess) {
        QSqlQuery scheduleQuery(m_db);
        scheduleQuery.prepare(R"(
            SELECT ds.start_time, ds.end_time, ds.max_appointments
            FROM doctor_schedules ds
            WHERE ds.doctor_username = :username 
            AND ds.day_of_week = CAST(strftime('%w', :appointment_date) AS INTEGER)
            AND ds.is_active = 1
        )");
        scheduleQuery.bindValue(":username", doctorUsername);
        scheduleQuery.bindValue(":appointment_date", appointmentDate);
        
        if (!scheduleQuery.exec() || !scheduleQuery.next()) {
            qDebug() << "Doctor has no schedule for this day, trying fallback:" << scheduleQuery.lastError().text();
            strictValidationSuccess = false;
        } else {
            QString startTime = scheduleQuery.value("start_time").toString();
            QString endTime = scheduleQuery.value("end_time").toString();
            int maxAppointments = scheduleQuery.value("max_appointments").toInt();
            
            // 3. 验证预约时间是否在医生工作时间内
            QTime apptTime = QTime::fromString(appointmentTime, "hh:mm");
            QTime workStart = QTime::fromString(startTime, "hh:mm");
            QTime workEnd = QTime::fromString(endTime, "hh:mm");
            
            if (apptTime < workStart || apptTime > workEnd) {
                qDebug() << "Appointment time is outside doctor's working hours, trying fallback";
                strictValidationSuccess = false;
            } else {
                // 4. 检查当天预约数量是否已满
                QSqlQuery countQuery(m_db);
                countQuery.prepare(R"(
                    SELECT COUNT(*) FROM appointments 
                    WHERE doctor_username = :username 
                    AND appointment_date = :date 
                    AND status != 'cancelled'
                )");
                countQuery.bindValue(":username", doctorUsername);
                countQuery.bindValue(":date", appointmentDate);
                
                if (!countQuery.exec() || !countQuery.next()) {
                    qDebug() << "Failed to count existing appointments, trying fallback:" << countQuery.lastError().text();
                    strictValidationSuccess = false;
                } else {
                    int currentAppointments = countQuery.value(0).toInt();
                    if (currentAppointments >= maxAppointments) {
                        qDebug() << "Doctor's appointment slots are full for this day, trying fallback";
                        strictValidationSuccess = false;
                    }
                }
            }
        }
    }
    
    // 如果严格验证失败，使用传入的数据作为回退
    if (!strictValidationSuccess) {
        doctorDepartment = appointmentData["department"].toString();
        consultationFee = appointmentData["fee"].toDouble();
    }
    
    // 5. 创建预约记录（无论是否通过严格验证）
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO appointments (
            patient_username, doctor_username, appointment_date, 
            appointment_time, department, chief_complaint, fee, status
        ) VALUES (
            :patient_username, :doctor_username, :appointment_date,
            :appointment_time, :department, :chief_complaint, :fee, 'pending'
        )
    )");
    
    query.bindValue(":patient_username", appointmentData["patient_username"].toString());
    query.bindValue(":doctor_username", doctorUsername);
    query.bindValue(":appointment_date", appointmentDate);
    query.bindValue(":appointment_time", appointmentTime);
    query.bindValue(":department", doctorDepartment);
    query.bindValue(":chief_complaint", appointmentData["chief_complaint"].toString());
    query.bindValue(":fee", consultationFee);
    
    if (!query.exec()) {
        qDebug() << "createAppointment error:" << query.lastError().text()
                 << " patient=" << appointmentData["patient_username"].toString()
                 << " doctor=" << doctorUsername
                 << " date=" << appointmentDate
                 << " time=" << appointmentTime
                 << " dept=" << doctorDepartment
                 << " fee=" << consultationFee;
        return false;
    }
    
    qDebug() << "Appointment created successfully for patient:" << appointmentData["patient_username"].toString() 
             << "with doctor:" << doctorUsername;
    return true;
}

bool DBManager::getAppointmentsByPatient(const QString& patientUsername, QJsonArray& appointments) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT a.id, a.doctor_username, a.appointment_date, a.appointment_time,
               a.status, a.department, a.chief_complaint, a.fee,
               d.name as doctor_name, d.title as doctor_title, d.specialization,
               d.phone as doctor_phone, d.consultation_fee, d.work_number,
               ds.start_time as schedule_start_time, ds.end_time as schedule_end_time,
               ds.max_appointments as schedule_max_appointments
        FROM appointments a
        LEFT JOIN doctors d ON a.doctor_username = d.username
        LEFT JOIN doctor_schedules ds ON (a.doctor_username = ds.doctor_username 
            AND ds.day_of_week = CAST(strftime('%w', a.appointment_date) AS INTEGER)
            AND ds.is_active = 1)
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
        appointment["doctor_specialization"] = query.value("specialization").toString();
        appointment["doctor_phone"] = query.value("doctor_phone").toString();
        appointment["doctor_consultation_fee"] = query.value("consultation_fee").toDouble();
        appointment["doctor_work_number"] = query.value("work_number").toString();
        appointment["schedule_start_time"] = query.value("schedule_start_time").toString();
        appointment["schedule_end_time"] = query.value("schedule_end_time").toString();
        appointment["schedule_max_appointments"] = query.value("schedule_max_appointments").toInt();
        appointments.append(appointment);
    }
    return true;
}

bool DBManager::getAppointmentsByDoctor(const QString& doctorUsername, QJsonArray& appointments) {
    qDebug() << "[DBManager] 查询医生预约，用户名:" << doctorUsername;
    
    // 添加文件日志
    QFile logFile("/tmp/db_debug.log");
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile);
        stream << "=== getAppointmentsByDoctor ===" << Qt::endl;
        stream << "医生用户名: " << doctorUsername << Qt::endl;
        stream << "数据库是否打开: " << m_db.isOpen() << Qt::endl;
        stream << "数据库文件: " << m_db.databaseName() << Qt::endl;
    }
    
    QSqlQuery query(m_db);
    QString sql = QString(R"(
        SELECT a.id, a.patient_username, a.appointment_date, a.appointment_time,
               a.status, a.department, a.chief_complaint, a.fee,
               p.name as patient_name, p.age as patient_age, p.phone as patient_phone,
               p.gender as patient_gender,
               d.name as doctor_name, d.title as doctor_title, d.specialization
        FROM appointments a
        LEFT JOIN patients p ON a.patient_username = p.username
        LEFT JOIN doctors d ON a.doctor_username = d.username
        WHERE a.doctor_username = '%1'
        ORDER BY a.appointment_date DESC, a.appointment_time DESC
    )").arg(doctorUsername);
    
    // 添加文件日志
    QFile logFileSQL("/tmp/db_debug.log");
    if (logFileSQL.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFileSQL);
        stream << "执行SQL: " << sql << Qt::endl;
    }

    if (!query.exec(sql)) {
        qDebug() << "getAppointmentsByDoctor error:" << query.lastError().text();
        
        // 添加文件日志
        QFile logFile2("/tmp/db_debug.log");
        if (logFile2.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QTextStream stream(&logFile2);
            stream << "SQL执行失败: " << query.lastError().text() << Qt::endl;
        }
        
        return false;
    }

    int count = 0;
    while (query.next()) {
        count++;
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
        appointment["patient_phone"] = query.value("patient_phone").toString();
        appointment["patient_gender"] = query.value("patient_gender").toString();
        appointment["doctor_name"] = query.value("doctor_name").toString();
        appointment["doctor_title"] = query.value("doctor_title").toString();
        appointment["doctor_specialization"] = query.value("specialization").toString();
        // 设置默认排班信息，因为可能没有排班数据
        appointment["schedule_start_time"] = "09:00";
        appointment["schedule_end_time"] = "17:00";
        appointment["schedule_max_appointments"] = 10;
        appointments.append(appointment);
    }
    qDebug() << "[DBManager] 查询完成，找到" << count << "条预约记录";
    
    // 添加文件日志
    QFile logFile3("/tmp/db_debug.log");
    if (logFile3.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&logFile3);
        stream << "查询完成，找到 " << count << " 条记录" << Qt::endl;
        stream << "========================" << Qt::endl;
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
    QString sql = QString(R"(
        INSERT INTO medical_records (
            appointment_id, patient_username, doctor_username, visit_date,
            chief_complaint, present_illness, past_history, physical_examination,
            diagnosis, treatment_plan, notes
        ) VALUES (
            %1, '%2', '%3', '%4', '%5', '%6', '%7', '%8', '%9', '%10', '%11'
        )
    )").arg(recordData["appointment_id"].toInt())
       .arg(recordData["patient_username"].toString())
       .arg(recordData["doctor_username"].toString())
       .arg(recordData["visit_date"].toString())
       .arg(recordData["chief_complaint"].toString())
       .arg(recordData["present_illness"].toString())
       .arg(recordData["past_history"].toString())
       .arg(recordData["physical_examination"].toString())
       .arg(recordData["diagnosis"].toString())
       .arg(recordData["treatment_plan"].toString())
       .arg(recordData["notes"].toString());
    
    if (!query.exec(sql)) {
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
    qDebug() << "createMedicalAdvice called with data:" << adviceData;
    
    QSqlQuery query(m_db);
    QString sql = QString(R"(
        INSERT INTO medical_advices (
            record_id, advice_type, content, priority
        ) VALUES (
            %1, '%2', '%3', '%4'
        )
    )").arg(adviceData["record_id"].toInt())
       .arg(adviceData["advice_type"].toString())
       .arg(adviceData["content"].toString())
       .arg(adviceData["priority"].toString());
    
    qDebug() << "createMedicalAdvice SQL:" << sql;
    
    if (!query.exec(sql)) {
        qDebug() << "createMedicalAdvice error:" << query.lastError().text();
        qDebug() << "createMedicalAdvice error type:" << query.lastError().type();
        qDebug() << "createMedicalAdvice error number:" << query.lastError().nativeErrorCode();
        return false;
    }
    
    qDebug() << "createMedicalAdvice success, inserted ID:" << query.lastInsertId().toInt();
    return true;
}

bool DBManager::getMedicalAdviceByRecord(int recordId, QJsonArray& advices) {
    QSqlQuery query(m_db);
    QString sql = QString(R"(
        SELECT id, advice_type, content, priority, created_at
        FROM medical_advices
        WHERE record_id = %1
        ORDER BY created_at DESC
    )").arg(recordId);
    
    if (!query.exec(sql)) {
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
    QString sql = QString(R"(
        INSERT INTO prescriptions (
            record_id, patient_username, doctor_username, prescription_date,
            total_amount, status, notes
        ) VALUES (
            %1, '%2', '%3', '%4', %5, '%6', '%7'
        )
    )").arg(prescriptionData["record_id"].toInt())
       .arg(prescriptionData["patient_username"].toString())
       .arg(prescriptionData["doctor_username"].toString())
       .arg(prescriptionData["prescription_date"].toString())
       .arg(prescriptionData["total_amount"].toDouble())
       .arg(prescriptionData["status"].toString())
       .arg(prescriptionData["notes"].toString());
    
    if (!query.exec(sql)) {
        qDebug() << "createPrescription error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::addPrescriptionItem(const QJsonObject& itemData) {
    QSqlQuery query(m_db);
    QString sql = QString(R"(
        INSERT INTO prescription_items (
            prescription_id, medication_id, quantity, dosage,
            frequency, duration, instructions, unit_price, total_price
        ) VALUES (
            %1, %2, %3, '%4', '%5', '%6', '%7', %8, %9
        )
    )").arg(itemData["prescription_id"].toInt())
       .arg(itemData["medication_id"].toInt())
       .arg(itemData["quantity"].toInt())
       .arg(itemData["dosage"].toString())
       .arg(itemData["frequency"].toString())
       .arg(itemData["duration"].toString())
       .arg(itemData["instructions"].toString())
       .arg(itemData["unit_price"].toDouble())
       .arg(itemData["total_price"].toDouble());
    
    if (!query.exec(sql)) {
        qDebug() << "addPrescriptionItem error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getPrescriptionsByPatient(const QString& patientUsername, QJsonArray& prescriptions) {
    QSqlQuery query(m_db);
    QString sql = QString(R"(
        SELECT p.id, p.record_id, p.doctor_username, p.prescription_date,
               p.total_amount, p.status, p.notes,
               d.name as doctor_name, d.title as doctor_title
        FROM prescriptions p
        LEFT JOIN doctors d ON p.doctor_username = d.username
        WHERE p.patient_username = '%1'
        ORDER BY p.prescription_date DESC
    )").arg(patientUsername);
    
    if (!query.exec(sql)) {
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
    if (!query.exec(R"(
        SELECT id, name, generic_name, category, manufacturer, specification,
               unit, price, stock_quantity, description, precautions, side_effects, 
               contraindications, image_path
        FROM medications 
        ORDER BY name
    )")) {
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
        medication["side_effects"] = query.value("side_effects").toString();
        medication["contraindications"] = query.value("contraindications").toString();
        medication["image_path"] = query.value("image_path").toString();
        medications.append(medication);
    }
    return true;
}

bool DBManager::searchMedications(const QString& keyword, QJsonArray& medications) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT id, name, generic_name, category, manufacturer, specification,
               unit, price, stock_quantity, description, precautions, side_effects,
               contraindications, image_path
        FROM medications
        WHERE name LIKE :keyword OR generic_name LIKE :keyword 
              OR category LIKE :keyword OR manufacturer LIKE :keyword
        ORDER BY name
    )");
    query.bindValue(":keyword", "%" + keyword + "%");
    
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
        medication["side_effects"] = query.value("side_effects").toString();
        medication["contraindications"] = query.value("contraindications").toString();
        medication["image_path"] = query.value("image_path").toString();
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
    if (!query.exec("SELECT username, name, department, title, specialization, consultation_fee, phone, email, work_number, max_patients_per_day FROM doctors")) {
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
        doctor["phone"] = query.value("phone").toString();
        doctor["email"] = query.value("email").toString();
        doctor["work_number"] = query.value("work_number").toString();
        doctor["max_patients_per_day"] = query.value("max_patients_per_day").toInt();
        doctors.append(doctor);
    }
    return true;
}

// 按科室获取医生列表
bool DBManager::getDoctorsByDepartment(const QString& department, QJsonArray& doctors) {
    QSqlQuery query(m_db);
    query.prepare("SELECT username, name, department, title, specialization, consultation_fee, phone, email, work_number, max_patients_per_day FROM doctors WHERE department = :department");
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
        doctor["phone"] = query.value("phone").toString();
        doctor["email"] = query.value("email").toString();
        doctor["work_number"] = query.value("work_number").toString();
        doctor["max_patients_per_day"] = query.value("max_patients_per_day").toInt();
        doctors.append(doctor);
    }
    return true;
}

// 住院管理实现
bool DBManager::createHospitalization(const QJsonObject& hospitalizationData) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO hospitalizations (
            patient_username, doctor_username, admission_date, ward, bed_number,
            diagnosis, treatment_plan, daily_cost, status, notes
        ) VALUES (
            :patient_username, :doctor_username, :admission_date, :ward, :bed_number,
            :diagnosis, :treatment_plan, :daily_cost, :status, :notes
        )
    )");
    
    query.bindValue(":patient_username", hospitalizationData["patient_username"].toString());
    query.bindValue(":doctor_username", hospitalizationData["doctor_username"].toString());
    query.bindValue(":admission_date", hospitalizationData["admission_date"].toString());
    // 兼容旧字段 ward_number
    {
        QString wardStr = hospitalizationData.contains("ward")
                               ? hospitalizationData.value("ward").toString()
                               : hospitalizationData.value("ward_number").toString();
        query.bindValue(":ward", wardStr);
    }
    query.bindValue(":bed_number", hospitalizationData["bed_number"].toString());
    query.bindValue(":diagnosis", hospitalizationData["diagnosis"].toString());
    query.bindValue(":treatment_plan", hospitalizationData["treatment_plan"].toString());
    query.bindValue(":daily_cost", hospitalizationData["daily_cost"].toDouble());
    
    QString status = hospitalizationData.contains("status") ? 
                     hospitalizationData["status"].toString() : "admitted";
    query.bindValue(":status", status);
    
    query.bindValue(":notes", hospitalizationData["notes"].toString());
    
    if (!query.exec()) {
        qDebug() << "createHospitalization error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::getHospitalizationsByPatient(const QString& patientUsername, QJsonArray& hospitalizations) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT h.id, h.doctor_username, h.admission_date, h.discharge_date, h.ward,
               h.bed_number, h.diagnosis, h.treatment_plan, h.daily_cost, h.total_cost,
               h.status, h.notes, h.created_at,
               d.name as doctor_name, d.title as doctor_title, d.department
        FROM hospitalizations h
        LEFT JOIN doctors d ON h.doctor_username = d.username
        WHERE h.patient_username = :patient_username
        ORDER BY h.admission_date DESC
    )");
    query.bindValue(":patient_username", patientUsername);

    if (!query.exec()) {
        qDebug() << "getHospitalizationsByPatient error:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QJsonObject hospitalization;
        hospitalization["id"] = query.value("id").toInt();
        hospitalization["doctor_username"] = query.value("doctor_username").toString();
        hospitalization["doctor_name"] = query.value("doctor_name").toString();
        hospitalization["doctor_title"] = query.value("doctor_title").toString();
        hospitalization["department"] = query.value("department").toString();
        hospitalization["admission_date"] = query.value("admission_date").toString();
        hospitalization["discharge_date"] = query.value("discharge_date").toString();
    hospitalization["ward"] = query.value("ward").toString();
    hospitalization["ward_number"] = hospitalization["ward"]; // 兼容旧字段
        hospitalization["bed_number"] = query.value("bed_number").toString();
        hospitalization["diagnosis"] = query.value("diagnosis").toString();
        hospitalization["treatment_plan"] = query.value("treatment_plan").toString();
        hospitalization["daily_cost"] = query.value("daily_cost").toDouble();
        hospitalization["total_cost"] = query.value("total_cost").toDouble();
        hospitalization["status"] = query.value("status").toString();
        hospitalization["notes"] = query.value("notes").toString();
        hospitalization["created_at"] = query.value("created_at").toString();
        hospitalizations.append(hospitalization);
    }
    return true;
}

bool DBManager::getHospitalizationsByDoctor(const QString& doctorUsername, QJsonArray& hospitalizations) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT h.id, h.patient_username, h.admission_date, h.discharge_date, h.ward,
               h.bed_number, h.diagnosis, h.treatment_plan, h.daily_cost, h.total_cost,
               h.status, h.notes, h.created_at,
               p.name as patient_name, p.age as patient_age, p.gender
        FROM hospitalizations h
        LEFT JOIN patients p ON h.patient_username = p.username
        WHERE h.doctor_username = :doctor_username
        ORDER BY h.admission_date DESC
    )");
    query.bindValue(":doctor_username", doctorUsername);

    if (!query.exec()) {
        qDebug() << "getHospitalizationsByDoctor error:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QJsonObject hospitalization;
        hospitalization["id"] = query.value("id").toInt();
        hospitalization["patient_username"] = query.value("patient_username").toString();
        hospitalization["patient_name"] = query.value("patient_name").toString();
        hospitalization["patient_age"] = query.value("patient_age").toInt();
        hospitalization["patient_gender"] = query.value("gender").toString();
        hospitalization["admission_date"] = query.value("admission_date").toString();
        hospitalization["discharge_date"] = query.value("discharge_date").toString();
    hospitalization["ward"] = query.value("ward").toString();
    hospitalization["ward_number"] = hospitalization["ward"]; // 兼容旧字段
        hospitalization["bed_number"] = query.value("bed_number").toString();
        hospitalization["diagnosis"] = query.value("diagnosis").toString();
        hospitalization["treatment_plan"] = query.value("treatment_plan").toString();
        hospitalization["daily_cost"] = query.value("daily_cost").toDouble();
        hospitalization["total_cost"] = query.value("total_cost").toDouble();
        hospitalization["status"] = query.value("status").toString();
        hospitalization["notes"] = query.value("notes").toString();
        hospitalization["created_at"] = query.value("created_at").toString();
        hospitalizations.append(hospitalization);
    }
    return true;
}

bool DBManager::getAllHospitalizations(QJsonArray& hospitalizations) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT h.id, h.patient_username, h.doctor_username, h.admission_date, h.discharge_date,
               h.ward, h.bed_number, h.diagnosis, h.treatment_plan, h.daily_cost, h.total_cost,
               h.status, h.notes, h.created_at,
               p.name as patient_name, p.age as patient_age, p.gender,
               d.name as doctor_name, d.title as doctor_title, d.department
        FROM hospitalizations h
        LEFT JOIN patients p ON h.patient_username = p.username
        LEFT JOIN doctors d ON h.doctor_username = d.username
        ORDER BY h.admission_date DESC
    )");

    if (!query.exec()) {
        qDebug() << "getAllHospitalizations error:" << query.lastError().text();
        return false;
    }

    while (query.next()) {
        QJsonObject hospitalization;
        hospitalization["id"] = query.value("id").toInt();
        hospitalization["patient_username"] = query.value("patient_username").toString();
        hospitalization["patient_name"] = query.value("patient_name").toString();
        hospitalization["patient_age"] = query.value("patient_age").toInt();
        hospitalization["patient_gender"] = query.value("gender").toString();
        hospitalization["doctor_username"] = query.value("doctor_username").toString();
        hospitalization["doctor_name"] = query.value("doctor_name").toString();
        hospitalization["doctor_title"] = query.value("doctor_title").toString();
        hospitalization["department"] = query.value("department").toString();
        hospitalization["admission_date"] = query.value("admission_date").toString();
        hospitalization["discharge_date"] = query.value("discharge_date").toString();
    hospitalization["ward"] = query.value("ward").toString();
    hospitalization["ward_number"] = hospitalization["ward"]; // 兼容旧字段
        hospitalization["bed_number"] = query.value("bed_number").toString();
        hospitalization["diagnosis"] = query.value("diagnosis").toString();
        hospitalization["treatment_plan"] = query.value("treatment_plan").toString();
        hospitalization["daily_cost"] = query.value("daily_cost").toDouble();
        hospitalization["total_cost"] = query.value("total_cost").toDouble();
        hospitalization["status"] = query.value("status").toString();
        hospitalization["notes"] = query.value("notes").toString();
        hospitalization["created_at"] = query.value("created_at").toString();
        hospitalizations.append(hospitalization);
    }
    return true;
}

bool DBManager::updateHospitalizationStatus(int hospitalizationId, const QString& status) {
    QSqlQuery query(m_db);
    query.prepare("UPDATE hospitalizations SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", hospitalizationId);
    
    if (!query.exec()) {
        qDebug() << "updateHospitalizationStatus error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::deleteHospitalization(int hospitalizationId) {
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM hospitalizations WHERE id = :id");
    query.bindValue(":id", hospitalizationId);
    
    if (!query.exec()) {
        qDebug() << "deleteHospitalization error:" << query.lastError().text();
        return false;
    }
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
    doctor1["work_number"] = "D001";
    doctor1["phone"] = "13800138001";
    doctor1["email"] = "zhangming@hospital.com";
    doctor1["experience_years"] = 15;
    doctor1["max_patients_per_day"] = 20;
    doctors.append(doctor1);
    
    QJsonObject doctor2;
    doctor2["username"] = "doctor2";
    doctor2["name"] = "李华";
    doctor2["department"] = "外科";
    doctor2["title"] = "副主任医师";
    doctor2["specialization"] = "骨科手术";
    doctor2["consultation_fee"] = 60.0;
    doctor2["work_number"] = "D002";
    doctor2["phone"] = "13800138002";
    doctor2["email"] = "lihua@hospital.com";
    doctor2["experience_years"] = 12;
    doctor2["max_patients_per_day"] = 15;
    doctors.append(doctor2);
    
    QJsonObject doctor3;
    doctor3["username"] = "doctor3";
    doctor3["name"] = "王芳";
    doctor3["department"] = "儿科";
    doctor3["title"] = "主治医师";
    doctor3["specialization"] = "儿童呼吸科";
    doctor3["consultation_fee"] = 45.0;
    doctor3["work_number"] = "D003";
    doctor3["phone"] = "13800138003";
    doctor3["email"] = "wangfang@hospital.com";
    doctor3["experience_years"] = 8;
    doctor3["max_patients_per_day"] = 25;
    doctors.append(doctor3);
    
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
            (username, name, department, title, specialization, consultation_fee, 
             work_number, phone, email, experience_years, max_patients_per_day) 
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        )");
        updateQuery.addBindValue(username);
        updateQuery.addBindValue(doctor["name"].toString());
        updateQuery.addBindValue(doctor["department"].toString());
        updateQuery.addBindValue(doctor["title"].toString());
        updateQuery.addBindValue(doctor["specialization"].toString());
        updateQuery.addBindValue(doctor["consultation_fee"].toDouble());
        updateQuery.addBindValue(doctor["work_number"].toString());
        updateQuery.addBindValue(doctor["phone"].toString());
        updateQuery.addBindValue(doctor["email"].toString());
        updateQuery.addBindValue(doctor["experience_years"].toInt());
        updateQuery.addBindValue(doctor["max_patients_per_day"].toInt());
        
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
    
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO medications (name, category, price, unit) VALUES (?, ?, ?, ?)");
    
    QStringList medications = {"阿司匹林", "感冒灵", "维生素C"};
    QStringList categories = {"心血管药物", "感冒药", "维生素"};
    QList<double> prices = {5.5, 12.0, 8.5};
    
    for (int i = 0; i < medications.size(); ++i) {
        query.addBindValue(medications[i]);
        query.addBindValue(categories[i]);
        query.addBindValue(prices[i]);
        query.addBindValue("盒");
        
        if (!query.exec()) {
            qDebug() << "Insert sample medication error:" << query.lastError().text();
        }
    }
}

// 医生排班管理实现
bool DBManager::getDoctorSchedules(const QString& doctorUsername, QJsonArray& schedules) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT day_of_week, start_time, end_time, max_appointments, is_active
        FROM doctor_schedules 
        WHERE doctor_username = :username
        ORDER BY day_of_week
    )");
    query.bindValue(":username", doctorUsername);
    
    if (!query.exec()) {
        qDebug() << "getDoctorSchedules error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject schedule;
        int dayOfWeek = query.value("day_of_week").toInt();
        
        // 转换星期几
        QString dayName;
        switch (dayOfWeek) {
            case 0: dayName = "周日"; break;
            case 1: dayName = "周一"; break;
            case 2: dayName = "周二"; break;
            case 3: dayName = "周三"; break;
            case 4: dayName = "周四"; break;
            case 5: dayName = "周五"; break;
            case 6: dayName = "周六"; break;
            default: dayName = "未知"; break;
        }
        
        schedule["day_of_week"] = dayOfWeek;
        schedule["day_name"] = dayName;
        schedule["start_time"] = query.value("start_time").toString();
        schedule["end_time"] = query.value("end_time").toString();
        schedule["max_appointments"] = query.value("max_appointments").toInt();
        schedule["is_active"] = query.value("is_active").toBool();
        
        schedules.append(schedule);
    }
    return true;
}

// 插入示例医生排班数据
void DBManager::insertSampleDoctorSchedules() {
    // 检查是否已经有排班数据
    QSqlQuery checkQuery(m_db);
    if (checkQuery.exec("SELECT COUNT(*) FROM doctor_schedules") && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        return; // 如果已有数据，不再插入
    }
    
    // 为 doctor1 (张明) 添加排班
    QStringList schedules = {
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor1', 1, '08:00', '17:00', 20, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor1', 2, '08:00', '17:00', 20, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor1', 3, '08:00', '17:00', 20, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor1', 4, '08:00', '17:00', 20, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor1', 5, '08:00', '12:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 1, '09:00', '18:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 2, '09:00', '18:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 3, '09:00', '18:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 4, '09:00', '18:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 5, '09:00', '18:00', 15, 1)",
        "INSERT INTO doctor_schedules (doctor_username, day_of_week, start_time, end_time, max_appointments, is_active) VALUES ('doctor2', 6, '09:00', '12:00', 10, 1)"
    };
    
    for (const QString& sql : schedules) {
        QSqlQuery query(m_db);
        if (!query.exec(sql)) {
            qDebug() << "Insert sample doctor schedule error:" << query.lastError().text();
        }
    }
}

// 获取医生的详细排班和预约统计信息
bool DBManager::getDoctorScheduleWithAppointmentStats(const QString& doctorUsername, QJsonArray& scheduleStats) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT 
            ds.day_of_week, ds.start_time, ds.end_time, ds.max_appointments, ds.is_active,
            d.name as doctor_name, d.title, d.department, d.specialization, d.consultation_fee,
            (SELECT COUNT(*) FROM appointments a 
             WHERE a.doctor_username = ds.doctor_username 
             AND CAST(strftime('%w', a.appointment_date) AS INTEGER) = ds.day_of_week
             AND a.status != 'cancelled'
             AND date(a.appointment_date) = date('now')
            ) as today_appointments,
            (SELECT COUNT(*) FROM appointments a 
             WHERE a.doctor_username = ds.doctor_username 
             AND CAST(strftime('%w', a.appointment_date) AS INTEGER) = ds.day_of_week
             AND a.status != 'cancelled'
             AND date(a.appointment_date) >= date('now')
             AND date(a.appointment_date) < date('now', '+7 days')
            ) as week_appointments
        FROM doctor_schedules ds
        LEFT JOIN doctors d ON ds.doctor_username = d.username
        WHERE ds.doctor_username = :username
        ORDER BY ds.day_of_week
    )");
    query.bindValue(":username", doctorUsername);
    
    if (!query.exec()) {
        qDebug() << "getDoctorScheduleWithAppointmentStats error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject scheduleInfo;
        int dayOfWeek = query.value("day_of_week").toInt();
        
        // 转换星期几
        QString dayName;
        switch (dayOfWeek) {
            case 0: dayName = "周日"; break;
            case 1: dayName = "周一"; break;
            case 2: dayName = "周二"; break;
            case 3: dayName = "周三"; break;
            case 4: dayName = "周四"; break;
            case 5: dayName = "周五"; break;
            case 6: dayName = "周六"; break;
            default: dayName = "未知"; break;
        }
        
        scheduleInfo["day_of_week"] = dayOfWeek;
        scheduleInfo["day_name"] = dayName;
        scheduleInfo["start_time"] = query.value("start_time").toString();
        scheduleInfo["end_time"] = query.value("end_time").toString();
        scheduleInfo["max_appointments"] = query.value("max_appointments").toInt();
        scheduleInfo["is_active"] = query.value("is_active").toBool();
        scheduleInfo["doctor_name"] = query.value("doctor_name").toString();
        scheduleInfo["doctor_title"] = query.value("title").toString();
        scheduleInfo["doctor_department"] = query.value("department").toString();
        scheduleInfo["doctor_specialization"] = query.value("specialization").toString();
        scheduleInfo["consultation_fee"] = query.value("consultation_fee").toDouble();
        scheduleInfo["today_appointments"] = query.value("today_appointments").toInt();
        scheduleInfo["week_appointments"] = query.value("week_appointments").toInt();
        scheduleInfo["available_slots_today"] = query.value("max_appointments").toInt() - query.value("today_appointments").toInt();
        
        scheduleStats.append(scheduleInfo);
    }
    return true;
}

// 获取所有医生的排班概览（用于患者端选择医生）
bool DBManager::getAllDoctorsScheduleOverview(QJsonArray& doctorsSchedule) {
    QSqlQuery query(m_db);
    query.prepare(R"(
        SELECT 
            d.username, d.name, d.title, d.department, d.specialization, 
            d.consultation_fee, d.max_patients_per_day,
            GROUP_CONCAT(
                CASE ds.day_of_week 
                    WHEN 0 THEN '周日' WHEN 1 THEN '周一' WHEN 2 THEN '周二' 
                    WHEN 3 THEN '周三' WHEN 4 THEN '周四' WHEN 5 THEN '周五' 
                    WHEN 6 THEN '周六' ELSE '未知' END || 
                ' (' || ds.start_time || '-' || ds.end_time || ')', 
                '; '
            ) as working_days,
            (SELECT COUNT(*) FROM appointments a 
             WHERE a.doctor_username = d.username 
             AND date(a.appointment_date) = date('now')
             AND a.status != 'cancelled'
            ) as today_total_appointments
        FROM doctors d
        LEFT JOIN doctor_schedules ds ON d.username = ds.doctor_username AND ds.is_active = 1
        GROUP BY d.username, d.name, d.title, d.department, d.specialization, 
                 d.consultation_fee, d.max_patients_per_day
        ORDER BY d.department, d.name
    )");
    
    if (!query.exec()) {
        qDebug() << "getAllDoctorsScheduleOverview error:" << query.lastError().text();
        return false;
    }
    
    while (query.next()) {
        QJsonObject doctorInfo;
        doctorInfo["username"] = query.value("username").toString();
        doctorInfo["name"] = query.value("name").toString();
        doctorInfo["title"] = query.value("title").toString();
        doctorInfo["department"] = query.value("department").toString();
        doctorInfo["specialization"] = query.value("specialization").toString();
        doctorInfo["consultation_fee"] = query.value("consultation_fee").toDouble();
        doctorInfo["max_patients_per_day"] = query.value("max_patients_per_day").toInt();
        doctorInfo["working_days"] = query.value("working_days").toString();
        doctorInfo["today_appointments"] = query.value("today_total_appointments").toInt();
        doctorInfo["available_slots_today"] = query.value("max_patients_per_day").toInt() - query.value("today_total_appointments").toInt();
        
        doctorsSchedule.append(doctorInfo);
    }
    return true;
}

