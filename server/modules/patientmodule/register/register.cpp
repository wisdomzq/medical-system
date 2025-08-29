#include "register.h"
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include "core/database/database.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

RegisterManager::RegisterManager(QObject* parent): QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &RegisterManager::onRequest);
    connect(this, &RegisterManager::businessResponseReady, this, [](QJsonObject payload){
        if (!payload.contains("request_uuid") && payload.contains("uuid"))
            payload.insert("request_uuid", payload.value("uuid").toString());
        MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, payload);
    });
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
            ds.reservedPatients = 0;   // 未统计已预约数量（需改DB才可精确）
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
    QString doctorUsername; QString department; double fee = 0.0;
    if (doctorId <= 0 || doctorId > schedules.size()) {
        errorMsg = QStringLiteral("医生序号无效");
        return false;
    }
    const DoctorSchedule &ds = schedules.at(doctorId - 1); // 序号从1开始
    doctorUsername = ds.jobNumber; // username
    department = ds.department;
    fee = ds.fee;

    QDir dir2(QCoreApplication::applicationDirPath());
    for (int i=0;i<4 && !dir2.exists("data");++i) dir2.cdUp();
    DBManager db(dir2.filePath("data/user.db"));
    const QDate today = QDate::currentDate();
    const QTime now = QTime::currentTime();
    QJsonObject appt;
    appt["patient_username"] = patientName;
    appt["doctor_username"] = doctorUsername;
    appt["appointment_date"] = today.toString("yyyy-MM-dd");
    appt["appointment_time"] = now.toString("HH:mm");
    appt["department"] = department;
    appt["chief_complaint"] = QString();
    appt["fee"] = fee;
    if (!db.createAppointment(appt)) {
        errorMsg = QStringLiteral("创建预约失败");
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
    if (action == "get_doctor_schedule") {
        auto list = getAllDoctorSchedules();
        QJsonArray arr; for (auto &ds : list) arr.append(doctorScheduleToJson(ds));
        QJsonObject resp; resp["type"] = "doctor_schedule_response"; resp["success"] = true; resp["data"] = arr;
        if (payload.contains("uuid")) resp["request_uuid"] = payload.value("uuid").toString();
        emit businessResponseReady(resp);
    } else if (action == "register_doctor") {
        QString patientName = payload.value("patientName").toString();
        QString err; bool ok = false;
        QList<DoctorSchedule> schedules = getAllDoctorSchedules();
        auto findByName = [&](const QString& target)->int { int idx=1; for (auto &ds : schedules) { if (ds.name == target) return idx; ++idx; } return -1; };
        auto findByUser = [&](const QString& target)->int { int idx=1; for (auto &ds : schedules) { if (ds.jobNumber == target) return idx; ++idx; } return -1; };
        int matchId = -1;
        if (payload.contains("doctor_name")) {
            QString docName = payload.value("doctor_name").toString();
            matchId = findByName(docName);
            if (matchId<0) matchId = findByUser(docName); // 兼容姓名==账号
        } else if (payload.contains("doctor_username")) {
            QString docUser = payload.value("doctor_username").toString();
            matchId = findByUser(docUser);
        } else if (payload.contains("doctorId")) {
            matchId = payload.value("doctorId").toInt();
        }
        if (matchId>0) {
            ok = registerPatient(matchId, patientName, err);
        } else {
            err = QStringLiteral("医生不存在");
        }
        QJsonObject resp; resp["type"] = "register_doctor_response"; resp["success"] = ok; if(!ok) resp["error"] = err; else resp["message"] = QStringLiteral("挂号成功");
        if (ok) {
            resp["doctor_name"] = payload.value("doctor_name").toString();
            resp["patient_name"] = patientName;
        }
        if (payload.contains("uuid")) resp["request_uuid"] = payload.value("uuid").toString();
        emit businessResponseReady(resp);
    }
}