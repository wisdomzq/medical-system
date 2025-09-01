#include "core/logging/logging.h"
#include "core/network/responsedispatcher.h"

using namespace Protocol;

ResponseDispatcher::ResponseDispatcher(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<Protocol::MessageType>("Protocol::MessageType");
    qRegisterMetaType<Protocol::Header>("Protocol::Header");
}

void ResponseDispatcher::onFrame(Header header, QByteArray payload)
{
    switch (header.type) {
    case MessageType::JsonResponse:
        handleJson(payload);
        break;
    case MessageType::ErrorResponse:
        handleError(payload);
        break;
    case MessageType::HeartbeatPong:
        emit heartbeatPong();
        break;
    case MessageType::FileDownloadChunk:
        emit fileChunkReceived(payload);
        break;
    case MessageType::FileDownloadComplete:
        emit fileDownloadCompleted(fromJsonPayload(payload));
        break;
    default:
        // 忽略非响应类（例如服务器误发的请求）
        break;
    }
}

void ResponseDispatcher::handleJson(const QByteArray& payload)
{
    const QJsonObject obj = fromJsonPayload(payload);
    qInfo() << "[ Client ] 收到JSON响应";
    Log::request("CommunicationClient", obj);
    emit jsonResponse(obj);
}

void ResponseDispatcher::handleError(const QByteArray& payload)
{
    const QJsonObject obj = fromJsonPayload(payload);
    emit errorResponse(obj.value("errorCode").toInt(), obj.value("errorMessage").toString());
}
