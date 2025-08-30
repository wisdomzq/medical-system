#pragma once

#include <QObject>
#include <QJsonObject>
#include "core/network/protocol.h"

// 客户端响应调度器：根据响应的 Header.type 分派到对应的信号
class ResponseDispatcher : public QObject {
    Q_OBJECT
public:
    explicit ResponseDispatcher(QObject* parent = nullptr);

public slots:
    void onFrame(Protocol::Header header, QByteArray payload);

signals:
    void jsonResponse(const QJsonObject& obj);
    void errorResponse(int code, const QString& msg);
    void heartbeatPong();

private:
    void handleJson(const QByteArray& payload);
    void handleError(const QByteArray& payload);
};
