#include <QCoreApplication>
#include <QDebug>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDateTime>

#include <protocol.h>
#include <server/communicationserver.h>
#include <server/messagerouter.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationServer server;
    QObject::connect(&server, &QTcpServer::acceptError, [](QAbstractSocket::SocketError err) {
        qWarning() << "Accept error:" << err;
    });

    // 示例业务：处理 echo
    QObject::connect(&MessageRouter::instance(), &MessageRouter::requestReceived,
                     [](QJsonObject payload) {
                         const QString uuid = payload.value("uuid").toString();
                         const QString action = payload.value("action").toString();
                         if (action == QStringLiteral("echo")) {
                             QJsonObject resp = payload;
                             resp.insert("serverTime", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
                             // 回发成功响应：在 payload 中携带 request_uuid
                             resp.insert("request_uuid", uuid);
                             MessageRouter::instance().onBusinessResponse(Protocol::MessageType::JsonResponse, resp);
                         } else {
                             QJsonObject err{{"errorCode", 404}, {"errorMessage", QStringLiteral("Unknown action")}};
                             err.insert("request_uuid", uuid);
                             MessageRouter::instance().onBusinessResponse(Protocol::MessageType::ErrorResponse, err);
                         }
                     });

    if (!server.listen(QHostAddress::Any, Protocol::SERVER_PORT)) {
        qCritical() << "Failed to listen on port" << Protocol::SERVER_PORT << ":" << server.errorString();
        return 1;
    }

    qInfo() << "Server listening on" << server.serverAddress().toString() << server.serverPort();
    return app.exec();
}
