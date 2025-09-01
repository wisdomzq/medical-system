#pragma once
#include <QDebug>
#include <QJsonObject>
#include <QString>

// 客户端统一日志工具，风格与服务端保持一致
namespace Log {

// 统一的响应日志（如需要在客户端侧记录服务端响应，可复用）
inline void response(const char* tag, const QJsonObject& resp)
{
    qInfo().noquote() << '[' << tag << ']' << "response"
                      << "type=" << resp.value("type").toString()
                      << "success=" << resp.value("success").toBool()
                      << "request_uuid=" << resp.value("request_uuid").toString();
}

// 统一的请求日志：打印 action, uuid，并可附加关键键值（最多三对以免刷屏）
inline void request(const char* tag, const QJsonObject& payload,
    const QString& k1 = QString(), const QString& v1 = QString(),
    const QString& k2 = QString(), const QString& v2 = QString(),
    const QString& k3 = QString(), const QString& v3 = QString())
{
    const QString action = payload.value("action").toString(payload.value("type").toString());
    const QString uuid = payload.value("uuid").toString();
    QString kv;
    auto appendKV = [&](const QString& k, const QString& v) { if (!k.isEmpty()) kv += QString(" %1=%2").arg(k, v); };
    appendKV(k1, v1);
    appendKV(k2, v2);
    appendKV(k3, v3);
    qInfo().noquote() << '[' << tag << ']' << "request"
                      << "action=" << action
                      << "uuid=" << uuid
                      << kv;
}

inline void result(const char* tag, bool success, const char* what = "result")
{
    qInfo().noquote() << '[' << tag << ']' << what << "success=" << success;
}

inline void error(const char* tag, const QString& message)
{
    qWarning().noquote() << '[' << tag << ']' << "error:" << message;
}

}
