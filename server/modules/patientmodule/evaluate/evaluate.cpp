#include "evaluate.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/database/database_config.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QDebug>

static const char* WALLET_TABLE_SQL = "CREATE TABLE IF NOT EXISTS patient_wallets (\n"
                                      " patient_username TEXT PRIMARY KEY,\n"
                                      " balance REAL DEFAULT 0,\n"
                                      " updated_at DATETIME DEFAULT CURRENT_TIMESTAMP\n"
                                      ")";

EvaluateModule::EvaluateModule(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &EvaluateModule::onRequest);
}

void EvaluateModule::onRequest(const QJsonObject &payload) {
    const QString action = payload.value("action").toString();
    if (action == "evaluate_get_config") return handleGetConfig(payload);
    if (action == "evaluate_recharge") return handleRecharge(payload);
}

void EvaluateModule::handleGetConfig(const QJsonObject &payload) {
    const QString patient = payload.value("patient_username").toString();
    double balance = ensureAndFetchBalance(patient);
    QJsonObject resp; resp["type"] = "evaluate_config_response"; resp["success"] = true;
    // 提供多份常用临床评估量表。若外网不可访问，前端仍可显示基本说明链接（本地 html 占位）。
    QJsonArray forms;
    auto addForm=[&](const QString &code,const QString &name,const QString &url){ QJsonObject o; o["code"]=code; o["name"]=name; o["url"]=url; forms.append(o); };
    addForm("phq9",   QStringLiteral("PHQ-9 抑郁自评"),        "https://www.phqscreeners.com/sites/g/files/g10060481/f/201412/PHQ-9_Chinese%28Simplified%29.pdf" );
    addForm("gad7",   QStringLiteral("GAD-7 焦虑自评"),        "https://www.phqscreeners.com/sites/g/files/g10060481/f/201412/GAD-7_Chinese%28Simplified%29.pdf" );
    addForm("sf36",   QStringLiteral("SF-36 健康生命质量"),    "https://orthotoolkit.com/sf-36/" );
    addForm("whoqol", QStringLiteral("WHOQOL-BREF 生活质量"),  "https://www.who.int/tools/whoqol/whoqol-bref" );
    addForm("audit",  QStringLiteral("AUDIT 酒精使用评估"),    "https://auditscreen.org/check-your-drinking/" );
    // 兜底：若将来需离线，前端可识别 url 以 local:// 前缀加载内置文本。
    if(forms.isEmpty()){
        addForm("phq9","PHQ-9 抑郁自评(离线)", "local://phq9.html");
    }
    resp["forms"] = forms; // 前端根据 name/url 渲染按钮
    resp["balance"] = balance;
    sendResponse(resp, payload);
}

void EvaluateModule::handleRecharge(const QJsonObject &payload) {
    const QString patient = payload.value("patient_username").toString();
    double amount = payload.value("amount").toDouble();
    QJsonObject resp; resp["type"] = "evaluate_recharge_response";
    if (amount <= 0) {
        resp["success"] = false; resp["error"] = QStringLiteral("充值金额必须大于0");
        return sendResponse(resp, payload);
    }
    double newBalance = 0; QString err;
    if (!updateBalance(patient, amount, newBalance, err)) {
        resp["success"] = false; resp["error"] = err;
    } else {
        resp["success"] = true; resp["balance"] = newBalance; resp["amount"] = amount;
    }
    sendResponse(resp, payload);
}

double EvaluateModule::ensureAndFetchBalance(const QString &patientUsername) {
    QSqlDatabase tempDb = QSqlDatabase::addDatabase("QSQLITE", QString("evaluate_conn_%1").arg(reinterpret_cast<quintptr>(this)) + QString::number(qrand()));
    tempDb.setDatabaseName(DatabaseConfig::getDatabasePath());
    if(!tempDb.open()) { qWarning() << "[EvaluateModule] 打开数据库失败:" << tempDb.lastError().text(); return 0.0; }
    {
        QSqlQuery create(tempDb);
        if(!create.exec(WALLET_TABLE_SQL)) {
            qWarning() << "[EvaluateModule] 创建钱包表失败:" << create.lastError().text();
        }
    }
    double balance = 0.0;
    {
        QSqlQuery sel(tempDb);
        sel.prepare("SELECT balance FROM patient_wallets WHERE patient_username=:p");
        sel.bindValue(":p", patientUsername);
        if(sel.exec() && sel.next()) {
            balance = sel.value(0).toDouble();
        } else {
            QSqlQuery ins(tempDb);
            ins.prepare("INSERT INTO patient_wallets (patient_username, balance) VALUES (:p, 0)");
            ins.bindValue(":p", patientUsername);
            if(!ins.exec()) qWarning() << "[EvaluateModule] 插入初始钱包失败:" << ins.lastError().text();
        }
    }
    tempDb.close();
    QSqlDatabase::removeDatabase(tempDb.connectionName());
    return balance;
}

bool EvaluateModule::updateBalance(const QString &patientUsername, double delta, double &newBalance, QString &err) {
    QSqlDatabase tempDb = QSqlDatabase::addDatabase("QSQLITE", QString("evaluate_conn_upd_%1").arg(reinterpret_cast<quintptr>(this)) + QString::number(qrand()));
    tempDb.setDatabaseName(DatabaseConfig::getDatabasePath());
    if(!tempDb.open()) { err = QStringLiteral("数据库打开失败"); return false; }
    {
        QSqlQuery create(tempDb); create.exec(WALLET_TABLE_SQL); // ignore error here
    }
    tempDb.transaction();
    QSqlQuery sel(tempDb); sel.prepare("SELECT balance FROM patient_wallets WHERE patient_username=:p"); sel.bindValue(":p", patientUsername);
    if(!sel.exec()) { err = sel.lastError().text(); tempDb.rollback(); tempDb.close(); QSqlDatabase::removeDatabase(tempDb.connectionName()); return false; }
    double current = 0.0; bool exists=false; if(sel.next()){ current = sel.value(0).toDouble(); exists=true; }
    if(!exists) {
        QSqlQuery ins(tempDb); ins.prepare("INSERT INTO patient_wallets (patient_username, balance) VALUES (:p, 0)"); ins.bindValue(":p", patientUsername);
        if(!ins.exec()) { err = ins.lastError().text(); tempDb.rollback(); tempDb.close(); QSqlDatabase::removeDatabase(tempDb.connectionName()); return false; }
    }
    newBalance = current + delta;
    QSqlQuery upd(tempDb); upd.prepare("UPDATE patient_wallets SET balance=:b, updated_at=CURRENT_TIMESTAMP WHERE patient_username=:p"); upd.bindValue(":b", newBalance); upd.bindValue(":p", patientUsername);
    if(!upd.exec()) { err = upd.lastError().text(); tempDb.rollback(); tempDb.close(); QSqlDatabase::removeDatabase(tempDb.connectionName()); return false; }
    tempDb.commit();
    tempDb.close(); QSqlDatabase::removeDatabase(tempDb.connectionName());
    return true;
}

void EvaluateModule::sendResponse(QJsonObject resp, const QJsonObject &orig) {
    if(orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
}
