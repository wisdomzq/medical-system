#include <server/clienthandler.h>

using namespace Protocol;

ClientHandler::ClientHandler(QObject* parent)
    : QObject(parent)
{
}

ClientHandler::~ClientHandler()
{
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

void ClientHandler::initialize(qintptr socketDescriptor)
{
    m_socket = new QTcpSocket(this);
    m_socket->setSocketDescriptor(socketDescriptor);
    qInfo() << "[Handler] 初始化完成，descriptor=" << socketDescriptor << ", 线程=" << QThread::currentThread();
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
}

void ClientHandler::sendMessage(MessageType type, const QJsonObject& obj)
{
    if (!m_socket)
        return;
    QByteArray payload = toJsonPayload(obj);
    QByteArray data = pack(type, payload);
    qInfo() << "[Handler] 发送消息 type=" << (quint16)type << ", 总字节=" << data.size();
    m_socket->write(data);
}

bool ClientHandler::parseFixedHeader(Header& out, int& headerBytesConsumed)
{
    if (!ensureBytesAvailable(FIXED_HEADER_SIZE))
        return false;

    QDataStream ds(m_buffer);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setVersion(QDataStream::Qt_5_15);

    quint32 magic;
    quint8 version;
    quint16 type;
    quint32 payloadSize;
    ds >> magic >> version >> type >> payloadSize;

    if (magic != MAGIC || version != VERSION) {
        qWarning() << "[Handler] 非法协议头，断开连接。magic/version=" << Qt::hex << magic << version;
        if (m_socket)
            m_socket->disconnectFromHost();
        return false;
    }
    if ((quint64)payloadSize > (quint64)MAX_PACKET_SIZE) {
        qWarning() << "[Handler] 报文过大，payload 超过上限，断开。大小=" << payloadSize;
        if (m_socket)
            m_socket->disconnectFromHost();
        return false;
    }

    out.magic = magic;
    out.version = version;
    out.type = (MessageType)type;
    out.payloadSize = payloadSize;

    headerBytesConsumed = FIXED_HEADER_SIZE;
    return true;
}

void ClientHandler::onReadyRead()
{
    if (!m_socket)
        return;
    QByteArray chunk = m_socket->readAll();
    qInfo() << "[Handler] 收到数据块 长度=" << chunk.size();
    m_buffer.append(chunk);

    while (true) {
        if (m_state == ParseState::WAITING_FOR_HEADER) {
            int consumed = 0;
            if (!parseFixedHeader(m_currentHeader, consumed))
                break;
            if (m_buffer.size() < consumed)
                break;
            m_buffer.remove(0, consumed);
            m_state = ParseState::WAITING_FOR_PAYLOAD;
            qInfo() << "[Handler] 已解析头部 type=" << (quint16)m_currentHeader.type
                    << ", payloadSize=" << m_currentHeader.payloadSize;
        }

        if (m_state == ParseState::WAITING_FOR_PAYLOAD) {
            if (!ensureBytesAvailable((int)m_currentHeader.payloadSize))
                break;
            QByteArray payload = m_buffer.left(m_currentHeader.payloadSize);
            m_buffer.remove(0, m_currentHeader.payloadSize);

            QJsonObject obj = fromJsonPayload(payload);
            emit requestReady(this, m_currentHeader, obj);

            // reset state
            m_state = ParseState::WAITING_FOR_HEADER;
            qInfo() << "[Handler] 一条完整报文处理完成";
        }
    }
}

void ClientHandler::onDisconnected()
{
    qInfo() << "[Handler] 客户端断开，准备销毁自身";
    this->deleteLater();
}
