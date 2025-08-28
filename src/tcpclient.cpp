#include "tcpclient.h"
#include <QJsonDocument>

TcpClient::TcpClient(QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
}

TcpClient::~TcpClient()
{
}

void TcpClient::connectToServer(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void TcpClient::sendRequest(const QJsonObject &request)
{
    m_socket->write(QJsonDocument(request).toJson());
}

void TcpClient::onConnected()
{
    emit connected();
}

void TcpClient::onDisconnected()
{
    emit disconnected();
}

void TcpClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    emit responseReceived(doc.object());
}
