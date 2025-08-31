#include "register.h"
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QDate>
#include "core/database/database.h"
#include "core/network/messagerouter.h"
#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

RegisterManager::RegisterManager(QObject* parent): QObject(parent) {
    qDebug() << "[RegisterManager] 构造函数被调用，开始连接信号槽...";
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &RegisterManager::onRequest, Qt::DirectConnection);
    qDebug() << "[RegisterManager] 信号槽连接完成";
    connect(this, &RegisterManager::businessResponse,
            &MessageRouter::instance(), &MessageRouter::onBusinessResponse,
            Qt::DirectConnection);
    qDebug() << "[RegisterManager] RegisterManager初始化完成";
}

// 获取所有医生排班信息
QList<DoctorSchedule> RegisterManager::getAllDoctorSchedules() {
    // 适配现有 doctors 表: username, name, department, title, specialization, consultation_fee
    // 数据库并无排班/上限/已预约等字段，这里用占位/默认值，不改动数据库。
    QList<DoctorSchedule> list;
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i=0;i<4 && !dir.exists("data");++i) dir.cdUp();
    DBManager db(dir.filePath("data/user.db"));
    QJsonArray doctors;
    if (db.getAllDoctors(doctors)) {
        QString today = QDate::currentDate().toString("yyyy-MM-dd");
        int idx = 1; // 生成临时 doctorId
        for (const auto &v : doctors) {
            QJsonObject o = v.toObject();
            DoctorSchedule ds{};
            ds.doctorId = idx++;                 // 临时自增ID
            ds.department = o.value("department").toString();
            ds.jobNumber = o.value("username").toString(); // 用 username 代替工号
            ds.name = o.value("name").toString();
            // profile: 使用 title + specialization 组合
            QString title = o.value("title").toString();
            QString spec = o.value("specialization").toString();
            ds.profile = title.isEmpty() ? spec : (spec.isEmpty()? title : title + " / " + spec);
            ds.workTime = "08:30-17:30"; // 默认工作时间占位
            ds.fee = o.value("consultation_fee").toDouble();
            ds.maxPatientsPerDay = 50; // 默认上限
            
            // 实时查询当天已预约数量
            QJsonArray appointments;
            if (db.getAppointmentsByDoctor(ds.jobNumber, appointments)) {
                int todayCount = 0;
                for (const auto& appt : appointments) {
                    QJsonObject appointmentObj = appt.toObject();
                    if (appointmentObj.value("appointment_date").toString() == today) {
                        todayCount++;
                    }
                }
                ds.reservedPatients = todayCount;
            } else {
                ds.reservedPatients = 0;   // 查询失败时默认为0
            }
            
            list.push_back(ds);
        }
        if (list.isEmpty()) {
            // fallback: 直接扫描 users 表 role=doctor
            QSqlDatabase conn = QSqlDatabase::addDatabase("QSQLITE", "register_fallback");
            QDir d(QCoreApplication::applicationDirPath());
            for (int i=0;i<4 && !d.exists("data");++i) d.cdUp();
            conn.setDatabaseName(d.filePath("data/user.db"));
            if (conn.open()) {
                QSqlQuery q(conn);
                if (q.exec("SELECT username FROM users WHERE role='doctor'")) {
                    int id=1; while (q.next()) {
                        DoctorSchedule ds{}; ds.doctorId = id++; ds.jobNumber = q.value(0).toString(); ds.name = ds.jobNumber;
                        ds.department = QString(); ds.profile = QString(); ds.workTime = "08:30-17:30"; ds.fee = 0.0; ds.maxPatientsPerDay=50; ds.reservedPatients=0; list.push_back(ds);
                    }
                }
            }
            conn.close();
            QSqlDatabase::removeDatabase("register_fallback");
        }
    }
    return list;
}

// 挂号逻辑
bool RegisterManager::registerPatient(int doctorId, const QString& patientName, QString& errorMsg) {
    // 使用内存中的排班列表（含 fallback），避免 doctors 表为空导致失败
    QList<DoctorSchedule> schedules = getAllDoctorSchedules();
    QString doctorUsername; QString department; double fee = 0.0; QString doctorName;
    
    if (doctorId <= 0 || doctorId > schedules.size()) {
        errorMsg = QStringLiteral("医生序号无效");
        return false;
    }
    
    const DoctorSchedule &ds = schedules.at(doctorId - 1); // 序号从1开始
    doctorUsername = ds.jobNumber; // username
    doctorName = ds.name;
    department = ds.department;
    fee = ds.fee;

    QDir dir2(QCoreApplication::applicationDirPath());
    for (int i=0;i<4 && !dir2.exists("data");++i) dir2.cdUp();
    DBManager db(dir2.filePath("data/user.db"));
    
    const QDate today = QDate::currentDate();
    const QTime now = QTime::currentTime();
    
    // 使用DBManager的createAppointment方法
    QJsonObject appt;
    appt["patient_username"] = patientName;
    appt["doctor_username"] = doctorUsername;
    appt["appointment_date"] = today.toString("yyyy-MM-dd");
    appt["appointment_time"] = now.toString("HH:mm");
    appt["department"] = department;
    appt["chief_complaint"] = QString("预约挂号 - %1").arg(doctorName);
    appt["fee"] = fee;
    
    if (!db.createAppointment(appt)) {
        errorMsg = QStringLiteral("创建预约失败，请稍后重试");
        return false;
    }
    
    return true;
}

// 将医生排班信息转为 JSON
QJsonObject RegisterManager::doctorScheduleToJson(const DoctorSchedule& ds) {
    QJsonObject obj;
    obj["doctorId"] = ds.doctorId;
    obj["department"] = ds.department;
    obj["jobNumber"] = ds.jobNumber; // 实际为 username
    obj["doctor_username"] = ds.jobNumber;
    obj["name"] = ds.name;
    obj["profile"] = ds.profile;
    obj["workTime"] = ds.workTime;
    obj["fee"] = ds.fee;
    obj["maxPatientsPerDay"] = ds.maxPatientsPerDay;
    obj["reservedPatients"] = ds.reservedPatients;
    obj["remainingSlots"] = ds.remainingSlots();
    return obj;
}

void RegisterManager::onRequest(const QJsonObject& payload) {
    const QString action = payload.value("action").toString(payload.value("type").toString());
    qDebug() << "[RegisterManager] 收到请求，action:" << action;
    qDebug() << "[RegisterManager] uuid=" << payload.value("uuid").toString()
             << ", doctor_name=" << payload.value("doctor_name").toString()
             << ", doctorId=" << payload.value("doctorId").toInt()
             << ", patientName=" << payload.value("patientName").toString();
    
    // 只处理挂号相关的动作
    if (action != "get_doctor_schedule" && action != "register_doctor") {
        return; // 不处理非挂号相关的请求
    }
    
    qDebug() << "[RegisterManager] 处理挂号相关请求:" << payload;
    
    if (action == "get_doctor_schedule") {
        auto list = getAllDoctorSchedules();
        QJsonArray arr; for (auto &ds : list) arr.append(doctorScheduleToJson(ds));
        QJsonObject resp; resp["type"] = "doctor_schedule_response"; resp["success"] = true; resp["data"] = arr;
    if (payload.contains("uuid")) resp["request_uuid"] = payload.value("uuid").toString();
    qDebug() << "[RegisterManager] 回传 doctor_schedule_response request_uuid=" << resp.value("request_uuid").toString();
    emit businessResponse(resp);
    } else if (action == "register_doctor") {
        qDebug() << "[RegisterManager] 处理挂号请求...";
        QString patientName = payload.value("patientName").toString();
        qDebug() << "[RegisterManager] 患者姓名:" << patientName;
        QString err; bool ok = false;
        QList<DoctorSchedule> schedules = getAllDoctorSchedules();
        
        auto findByName = [&](const QString& target)->int { 
            int idx=1; 
            for (auto &ds : schedules) { 
                if (ds.name == target || ds.jobNumber == target) return idx; 
                ++idx; 
            } 
            return -1; 
        };
        
        auto findByUser = [&](const QString& target)->int { 
            int idx=1; 
            for (auto &ds : schedules) { 
                if (ds.jobNumber == target) return idx; 
                ++idx; 
            } 
            return -1; 
        };
        
        int matchId = -1;
        
        // 优先使用明确的医生ID
        if (payload.contains("doctorId")) {
            matchId = payload.value("doctorId").toInt();
        } else if (payload.contains("doctor_name")) {
            QString docName = payload.value("doctor_name").toString();
            matchId = findByName(docName);
        } else if (payload.contains("doctor_username")) {
            QString docUser = payload.value("doctor_username").toString();
            matchId = findByUser(docUser);
        }
        
        if (matchId > 0 && matchId <= schedules.size()) {
            qDebug() << "[RegisterManager] 找到医生，序号:" << matchId << "开始挂号...";
            ok = registerPatient(matchId, patientName, err);
            qDebug() << "[RegisterManager] 挂号结果:" << ok << "错误信息:" << err;
        } else {
            err = QStringLiteral("医生不存在或序号无效");
            qDebug() << "[RegisterManager] 医生不存在，matchId:" << matchId << "schedules.size():" << schedules.size();
        }
        
        QJsonObject resp; 
        resp["type"] = "register_doctor_response"; 
        resp["success"] = ok; 
        
        if(!ok) {
            resp["error"] = err;
        } else {
            resp["message"] = QStringLiteral("挂号成功");
            resp["doctor_name"] = payload.value("doctor_name").toString();
            resp["patient_name"] = patientName;
        }
        
    if (payload.contains("uuid")) { resp["request_uuid"] = payload.value("uuid").toString(); }
    qDebug() << "[RegisterManager] 回传 register_doctor_response success=" << ok
         << ", request_uuid=" << resp.value("request_uuid").toString();
        
    emit businessResponse(resp);
    }
}