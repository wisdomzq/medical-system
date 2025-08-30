#ifndef ADVICE_H
#define ADVICE_H

#include <QObject>
#include <QJsonObject>
#include "core/network/protocol.h"

class AdviceModule : public QObject {
    Q_OBJECT

public:
    explicit AdviceModule(QObject* parent = nullptr);
signals:
    void businessResponse(Protocol::MessageType type, QJsonObject payload);

public:
    // 处理医嘱相关请求
    QJsonObject handleAdviceRequest(const QJsonObject& request);

private:
    // 获取患者医嘱列表
    QJsonObject getAdvicesByPatient(const QString& patientUsername);
    
    // 获取医嘱详细信息
    QJsonObject getAdviceDetails(int adviceId);

public slots:
    void onRequestReceived(const QJsonObject& payload);
};

#endif // ADVICE_H
