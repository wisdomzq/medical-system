#include <QCoreApplication>
#include <QDebug>
#include <QTextStream>
#include <QTimer>

#include <client/communicationclient.h>
#include <protocol.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    qRegisterMetaType<Protocol::Header>("Protocol::Header");

    CommunicationClient client;

    QObject::connect(&client, &CommunicationClient::connected, []() { qInfo() << "Client connected"; });
    QObject::connect(&client, &CommunicationClient::disconnected, []() { qInfo() << "Client disconnected"; });
    QTimer::singleShot(1500, [&client]() {
        qInfo() << "Sending demo JSON";
        client.sendJson(QJsonObject { { "action", "echo" }, { "value", 42 } });
    });
    QObject::connect(&client, &CommunicationClient::jsonReceived, [](const QJsonObject& obj) {
        qInfo() << "JSON response:" << QJsonDocument(obj).toJson(QJsonDocument::Compact);
    });
    QObject::connect(&client, &CommunicationClient::errorOccurred, [](int c, const QString& m) {
        qWarning() << "Error" << c << m;
    });

    QString host = "127.0.0.1";
    quint16 port = Protocol::SERVER_PORT;
    if (app.arguments().size() >= 2)
        host = app.arguments().at(1);
    if (app.arguments().size() >= 3)
        port = app.arguments().at(2).toUShort();

    client.connectToServer(host, port);

    return app.exec();
}
