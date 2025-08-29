#include "communicationclient.h"
#include <client/streamparser.h>
#include <client/responsedispatcher.h>

using namespace Protocol;

CommunicationClient::CommunicationClient(QObject* parent)
    : QObject(parent)
{
    // 组件
    m_parser = new StreamFrameParser(this);
    m_dispatcher = new ResponseDispatcher(this);

    connect(&m_socket, &QTcpSocket::connected, this, &CommunicationClient::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &CommunicationClient::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this, &CommunicationClient::onReadyRead);

    m_pingTimer.setInterval(HEARTBEAT_INTERVAL_MS);
    connect(&m_pingTimer, &QTimer::timeout, this, &CommunicationClient::sendHeartbeat);

    m_pongTimeoutTimer.setSingleShot(true);
    connect(&m_pongTimeoutTimer, &QTimer::timeout, this, &CommunicationClient::onHeartbeatTimeout);

    // 连接解析器->调度器
    connect(m_parser, &StreamFrameParser::frameReady, m_dispatcher, &ResponseDispatcher::onFrame);
    connect(m_parser, &StreamFrameParser::protocolError, this, [this](const QString& msg) {
        qWarning() << "[Client] 协议错误:" << msg << ", 主动断开";
        m_socket.abort();
    });

    // 调度器->对外信号
    connect(m_dispatcher, &ResponseDispatcher::jsonResponse, this, &CommunicationClient::jsonReceived);
    connect(m_dispatcher, &ResponseDispatcher::errorResponse, this, &CommunicationClient::errorOccurred);
    connect(m_dispatcher, &ResponseDispatcher::heartbeatPong, this, [this]() {
        qInfo() << "[Client] 收到心跳PONG";
        if (m_pongTimeoutTimer.isActive())
            m_pongTimeoutTimer.stop();
    });
}

void CommunicationClient::connectToServer(const QString& host, quint16 port)
{
    m_host = host;
    m_port = port;
    qInfo() << "[Client] 尝试连接服务器:" << host << ":" << port;
    m_socket.connectToHost(host, port);
}

void CommunicationClient::sendJson(const QJsonObject& obj)
{
    QByteArray payload = toJsonPayload(obj);
    QByteArray data = pack(MessageType::JsonRequest, payload);
    qInfo() << "[Client] 发送JSON请求，大小=" << data.size() << "字节";
    m_socket.write(data);
}

void CommunicationClient::onConnected()
{
    qInfo() << "[Client] 已连接服务器";
    emit connected();
    m_reconnectDelay = 1000;
    m_pingTimer.start();
}

void CommunicationClient::onDisconnected()
{
    qWarning() << "[Client] 与服务器断开，准备重连，当前重连间隔(ms)=" << m_reconnectDelay;
    emit disconnected();
    m_pingTimer.stop();
    m_pongTimeoutTimer.stop();
    QTimer::singleShot(m_reconnectDelay, this, [this]() {
        if (m_host.isEmpty() || m_port == 0)
            return;
        qInfo() << "[Client] 发起重连:" << m_host << ":" << m_port;
        m_socket.abort();
        m_socket.connectToHost(m_host, m_port);
    });
    if (m_reconnectDelay < 60000)
        m_reconnectDelay *= 2;
}

void CommunicationClient::onReadyRead()
{
    QByteArray chunk = m_socket.readAll();
    qInfo() << "[Client] 收到数据块，长度=" << chunk.size();
    m_parser->append(chunk);
}

void CommunicationClient::sendHeartbeat()
{
    QByteArray payload = toJsonPayload(QJsonObject { { "ping", true } });
    QByteArray data = pack(MessageType::HeartbeatPing, payload);
    qInfo() << "[Client] 发送心跳PING";
    m_socket.write(data);
    m_pongTimeoutTimer.start(HEARTBEAT_TIMEOUT_MS);
}

void CommunicationClient::onHeartbeatTimeout()
{
    qWarning() << "[Client] 心跳超时，主动断开触发重连";
    m_socket.abort(); // 触发重连
}
