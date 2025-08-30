#include "loginrouter.h"
#include "loginmodule.h"
#include "core/network/src/server/messagerouter.h"
#include "core/network/src/protocol.h"
#include "core/logging/logging.h"
#include <QDebug>

LoginRouter::LoginRouter(QObject *parent):QObject(parent) {
    connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
            this, &LoginRouter::onRequest);
}

void LoginRouter::onRequest(const QJsonObject &payload) {
    const QString action = payload.value("action").toString();
    if (action != "login" && action != "register") return;
    const QString uuid = payload.value("uuid").toString();
    const QString username = payload.value("username").toString();
    Log::request("LoginRouter", payload, "user", username);
    LoginModule lm; // 复用现有逻辑（内部自管 DB 生命周期）
    QJsonObject resp = (action == "login")
        ? lm.handleLogin(payload)
        : lm.handleRegister(payload);
    // 此处先不打印 response，等到补充了 request_uuid 后在 reply 中统一打印
    reply(resp, payload);
}

void LoginRouter::reply(QJsonObject resp, const QJsonObject &orig) {
    if (orig.contains("uuid")) resp["request_uuid"] = orig.value("uuid").toString();
    Log::response("LoginRouter", resp);
    MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
}
