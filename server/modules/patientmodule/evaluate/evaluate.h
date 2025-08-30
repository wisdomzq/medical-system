#pragma once
#include <QObject>
#include <QJsonObject>
#include "core/network/src/protocol.h"

// EvaluateModule: 提供健康评估相关接口
// 动作:
// 1. evaluate_get_config -> 返回问卷链接 & 当前余额
// 2. evaluate_recharge   -> 充值并返回最新余额
class EvaluateModule : public QObject {
    Q_OBJECT
public:
    explicit EvaluateModule(QObject *parent=nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);
private slots:
    void onRequest(const QJsonObject &payload);
private:
    void handleGetConfig(const QJsonObject &payload);
    void handleRecharge(const QJsonObject &payload);
    void sendResponse(QJsonObject resp, const QJsonObject &orig);
    double ensureAndFetchBalance(const QString &patientUsername);
    bool updateBalance(const QString &patientUsername, double delta, double &newBalance, QString &err);
};
