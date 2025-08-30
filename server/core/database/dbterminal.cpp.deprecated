#include "dbterminal.h"
#include "database.h"
#include "core/logging/logging.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <iostream>
#include <iomanip>

DBTerminal::DBTerminal(DBManager* dbManager, QObject *parent)
    : QObject(parent), m_dbManager(dbManager)
{
    m_displayTimer = new QTimer(this);
    if (!connect(m_displayTimer, &QTimer::timeout, this, &DBTerminal::updateDisplay)) {
        Log::error("DBTerminal", "Failed to connect QTimer::timeout to DBTerminal::updateDisplay");
    }
}

void DBTerminal::startMonitoring(int intervalMs) {
    m_displayTimer->start(intervalMs);
    qDebug() << "数据库监控已启动，更新间隔:" << intervalMs << "毫秒";
}

void DBTerminal::stopMonitoring() {
    m_displayTimer->stop();
    qDebug() << "数据库监控已停止";
}

void DBTerminal::updateDisplay() {
    qDebug() << "\n" << "数据库状态更新 -" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    showDatabaseOverview();
}

void DBTerminal::showDatabaseOverview() {
    printSeparator();
    qDebug() << "=== 数据库概览 ===";
    printSeparator();
    
    // 检查数据库连接状态
    QSqlDatabase db = m_dbManager->getDatabase();
    if (!db.isOpen()) {
        qDebug() << "错误: 数据库连接未打开";
        qDebug() << "数据库错误:" << db.lastError().text();
        printSeparator();
        return;
    }

    QSqlQuery query(db); // 使用获取到的数据库连接
    
    // 统计各表记录数
    QStringList tables = {"users", "doctors", "patients", "appointments", 
                         "medical_records", "medical_advices", "prescriptions", "medications"};
    QStringList tableNames = {"用户", "医生", "患者", "预约", "病例", "医嘱", "处方", "药品"};
    
    qDebug() << QString("表名").leftJustified(15) << QString("记录数").rightJustified(10) << QString("最后更新").rightJustified(20);
    printSeparator(50);
    
    for (int i = 0; i < tables.size(); ++i) {
        query.exec(QString("SELECT COUNT(*) FROM %1").arg(tables[i]));
        int count = 0;
        if (query.next()) {
            count = query.value(0).toInt();
        }
        
        QString lastUpdate = "N/A";
        if (tables[i] != "medications") {
            query.exec(QString("SELECT MAX(updated_at) FROM %1 WHERE updated_at IS NOT NULL").arg(tables[i]));
            if (query.next() && !query.value(0).isNull()) {
                lastUpdate = query.value(0).toString();
            }
        }
        
        qDebug() << QString(tableNames[i]).leftJustified(15) 
                 << QString::number(count).rightJustified(10)
                 << lastUpdate.rightJustified(20);
    }
    printSeparator();
}

void DBTerminal::showUsers() {
    printSeparator();
    qDebug() << "=== 用户账户信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec("SELECT username, role, created_at FROM users ORDER BY created_at DESC")) {
        printTableHeader({"用户名", "角色", "创建时间"});
        while (query.next()) {
            printTableRow({
                query.value("username").toString(),
                query.value("role").toString(),
                query.value("created_at").toString()
            });
        }
    } else {
        qDebug() << "查询用户失败:" << query.lastError().text();
    }
    printSeparator();
}

void DBTerminal::showDoctors() {
    printSeparator();
    qDebug() << "=== 医生基本信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT username, name, department, title, phone, email, consultation_fee
        FROM doctors ORDER BY department, name
    )")) {
        printTableHeader({"工号", "姓名", "科室", "职称", "电话", "邮箱", "挂号费"});
        while (query.next()) {
            printTableRow({
                query.value("username").toString(),
                query.value("name").toString(),
                query.value("department").toString(),
                query.value("title").toString(),
                query.value("phone").toString(),
                query.value("email").toString(),
                QString::number(query.value("consultation_fee").toDouble(), 'f', 2)
            });
        }
    }
    printSeparator();
}

void DBTerminal::showPatients() {
    printSeparator();
    qDebug() << "=== 患者基本信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT username, name, age, gender, phone, email
        FROM patients ORDER BY name
    )")) {
        printTableHeader({"用户名", "姓名", "年龄", "性别", "电话", "邮箱"});
        while (query.next()) {
            printTableRow({
                query.value("username").toString(),
                query.value("name").toString(),
                QString::number(query.value("age").toInt()),
                query.value("gender").toString(),
                query.value("phone").toString(),
                query.value("email").toString()
            });
        }
    }
    printSeparator();
}

void DBTerminal::showAppointments() {
    printSeparator();
    qDebug() << "=== 预约信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT a.id, p.name as patient_name, d.name as doctor_name,
               a.appointment_date, a.appointment_time, a.status, a.fee
        FROM appointments a
        LEFT JOIN patients p ON a.patient_username = p.username
        LEFT JOIN doctors d ON a.doctor_username = d.username
        ORDER BY a.appointment_date DESC, a.appointment_time DESC
        LIMIT 20
    )")) {
        printTableHeader({"ID", "患者", "医生", "日期", "时间", "状态", "费用"});
        while (query.next()) {
            printTableRow({
                QString::number(query.value("id").toInt()),
                query.value("patient_name").toString(),
                query.value("doctor_name").toString(),
                query.value("appointment_date").toString(),
                query.value("appointment_time").toString(),
                query.value("status").toString(),
                QString::number(query.value("fee").toDouble(), 'f', 2)
            });
        }
    }
    printSeparator();
}

void DBTerminal::showMedicalRecords() {
    printSeparator();
    qDebug() << "=== 病例信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT mr.id, p.name as patient_name, d.name as doctor_name,
               mr.visit_date, mr.chief_complaint, mr.diagnosis
        FROM medical_records mr
        LEFT JOIN patients p ON mr.patient_username = p.username
        LEFT JOIN doctors d ON mr.doctor_username = d.username
        ORDER BY mr.visit_date DESC
        LIMIT 15
    )")) {
        printTableHeader({"ID", "患者", "医生", "就诊时间", "主诉", "诊断"});
        while (query.next()) {
            printTableRow({
                QString::number(query.value("id").toInt()),
                query.value("patient_name").toString(),
                query.value("doctor_name").toString(),
                query.value("visit_date").toString(),
                query.value("chief_complaint").toString().left(20) + "...",
                query.value("diagnosis").toString().left(20) + "..."
            });
        }
    }
    printSeparator();
}

void DBTerminal::showMedicalAdvices() {
    printSeparator();
    qDebug() << "=== 医嘱信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT ma.id, ma.record_id, ma.advice_type, ma.content, ma.priority, ma.created_at
        FROM medical_advices ma
        ORDER BY ma.created_at DESC
        LIMIT 15
    )")) {
        printTableHeader({"ID", "病例ID", "类型", "内容", "优先级", "创建时间"});
        while (query.next()) {
            printTableRow({
                QString::number(query.value("id").toInt()),
                QString::number(query.value("record_id").toInt()),
                query.value("advice_type").toString(),
                query.value("content").toString().left(30) + "...",
                query.value("priority").toString(),
                query.value("created_at").toString()
            });
        }
    }
    printSeparator();
}

void DBTerminal::showPrescriptions() {
    printSeparator();
    qDebug() << "=== 处方信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT p.id, pt.name as patient_name, d.name as doctor_name,
               p.prescription_date, p.total_amount, p.status
        FROM prescriptions p
        LEFT JOIN patients pt ON p.patient_username = pt.username
        LEFT JOIN doctors d ON p.doctor_username = d.username
        ORDER BY p.prescription_date DESC
        LIMIT 15
    )")) {
        printTableHeader({"ID", "患者", "医生", "开方时间", "总金额", "状态"});
        while (query.next()) {
            printTableRow({
                QString::number(query.value("id").toInt()),
                query.value("patient_name").toString(),
                query.value("doctor_name").toString(),
                query.value("prescription_date").toString(),
                QString::number(query.value("total_amount").toDouble(), 'f', 2),
                query.value("status").toString()
            });
        }
    }
    printSeparator();
}

void DBTerminal::showMedications() {
    printSeparator();
    qDebug() << "=== 药品信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    if (query.exec(R"(
        SELECT id, name, category, manufacturer, specification, 
               unit, price, stock_quantity
        FROM medications 
        ORDER BY category, name
        LIMIT 20
    )")) {
        printTableHeader({"ID", "药品名", "类别", "生产商", "规格", "单位", "价格", "库存"});
        while (query.next()) {
            printTableRow({
                QString::number(query.value("id").toInt()),
                query.value("name").toString(),
                query.value("category").toString(),
                query.value("manufacturer").toString(),
                query.value("specification").toString(),
                query.value("unit").toString(),
                QString::number(query.value("price").toDouble(), 'f', 2),
                QString::number(query.value("stock_quantity").toInt())
            });
        }
    }
    printSeparator();
}

void DBTerminal::showStatistics() {
    printSeparator();
    qDebug() << "=== 系统统计信息 ===";
    printSeparator();
    
    QSqlQuery query(m_dbManager->getDatabase());
    
    // 今日预约统计
    if (query.exec("SELECT COUNT(*) FROM appointments WHERE appointment_date = DATE('now', 'localtime')")) {
        if (query.next()) {
            qDebug() << "今日预约数:" << query.value(0).toInt();
        }
    }
    
    // 本月预约统计
    if (query.exec(R"(
        SELECT COUNT(*) FROM appointments 
        WHERE strftime('%Y-%m', appointment_date) = strftime('%Y-%m', 'now', 'localtime')
    )")) {
        if (query.next()) {
            qDebug() << "本月预约数:" << query.value(0).toInt();
        }
    }
    
    // 活跃医生数
    if (query.exec(R"(
        SELECT COUNT(DISTINCT doctor_username) FROM appointments 
        WHERE appointment_date >= DATE('now', '-30 days')
    )")) {
        if (query.next()) {
            qDebug() << "近30天活跃医生数:" << query.value(0).toInt();
        }
    }
    
    // 科室预约分布
    qDebug() << "\n科室预约分布:";
    if (query.exec(R"(
        SELECT department, COUNT(*) as count 
        FROM appointments a
        LEFT JOIN doctors d ON a.doctor_username = d.username
        WHERE a.appointment_date >= DATE('now', '-7 days')
        GROUP BY department
        ORDER BY count DESC
    )")) {
        while (query.next()) {
            QString dept = query.value("department").toString();
            if (dept.isEmpty()) dept = "未分类";
            qDebug() << QString("  %1: %2").arg(dept).arg(query.value("count").toInt());
        }
    }
    
    printSeparator();
}

void DBTerminal::printTableHeader(const QStringList& headers) {
    QString headerLine;
    for (const QString& header : headers) {
        headerLine += header.leftJustified(15) + " ";
    }
    qDebug() << headerLine;
    
    QString separator;
    for (int i = 0; i < headers.size(); ++i) {
        separator += QString(15, '-') + " ";
    }
    qDebug() << separator;
}

void DBTerminal::printTableRow(const QStringList& data) {
    QString dataLine;
    for (const QString& item : data) {
        QString truncated = item.length() > 14 ? item.left(11) + "..." : item;
        dataLine += truncated.leftJustified(15) + " ";
    }
    qDebug() << dataLine;
}

void DBTerminal::printSeparator(int width) {
    qDebug() << QString(width, '=');
}