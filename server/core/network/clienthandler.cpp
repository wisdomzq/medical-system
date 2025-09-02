#include "core/network/clienthandler.h"
#include "core/logging/logging.h"
#include "core/network/filetransferprocessor.h"
#include "core/network/streamparser.h"
#include <QDir>
#include <QFile>

using namespace Protocol;

ClientHandler::ClientHandler(QObject* parent)
    : QObject(parent)
{
    m_file = new FileTransferProcessor("files");
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
    qInfo() << "[ Handler ] 初始化完成，descriptor=" << socketDescriptor << ", 线程=" << QThread::currentThread();
    if (!connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead)) {
        Log::error("ClientHandler", "Failed to connect QTcpSocket::readyRead to ClientHandler::onReadyRead");
    }
    // 初始化解析器
    m_parser = new StreamFrameParser(this);
    connect(m_parser, &StreamFrameParser::frameReady, this, [this](Header header, QByteArray payload) {
        this->m_currentHeader = header;
        // 复用原有帧处理逻辑：文件传输的 Meta/Complete/DownloadRequest 也携带 JSON，需要解析
        const bool isJsonPayload = (
            header.type == MessageType::JsonRequest ||
            header.type == MessageType::ErrorResponse ||
            header.type == MessageType::JsonResponse ||
            header.type == MessageType::FileUploadMeta ||
            header.type == MessageType::FileUploadComplete ||
            header.type == MessageType::FileDownloadRequest
        );
        QJsonObject obj = isJsonPayload ? fromJsonPayload(payload) : QJsonObject{};

        switch (header.type) {
        case MessageType::JsonRequest:
            emit requestJsonReady(this, obj);
            break;
        case MessageType::HeartbeatPing:
            sendMessage(MessageType::HeartbeatPong, QJsonObject());
            break;
        case MessageType::FileUploadMeta: {
            qInfo() << "[ Handler ] 收到 FileUploadMeta" << obj;
            QJsonObject out;
            if (!m_file->beginUpload(obj, out)) {
                sendMessage(MessageType::FileTransferError, out);
            } else {
                sendMessage(MessageType::JsonResponse, out);
            }
            break;
        }
        case MessageType::FileUploadChunk: {
            // 二进制分片
            QJsonObject err;
            if (!m_file->appendChunk(payload, err)) {
                sendMessage(MessageType::FileTransferError, err);
            }
            break;
        }
        case MessageType::FileUploadComplete: {
            qInfo() << "[ Handler ] 收到 FileUploadComplete" << obj;
            QJsonObject result;
            if (!m_file->finishUpload(result)) {
                sendMessage(MessageType::FileTransferError, result);
            } else {
                sendMessage(MessageType::JsonResponse, result);
            }
            break;
        }
        case MessageType::FileDownloadRequest: {
            qInfo() << "[ Handler ] 收到 FileDownloadRequest" << obj;
            QJsonObject complete;
            bool ok = m_file->downloadWhole(obj, [this](const QByteArray& data) {
                this->sendBinary(MessageType::FileDownloadChunk, data);
            }, complete);
            if (!ok) {
                sendMessage(MessageType::FileTransferError, complete);
            } else {
                sendMessage(MessageType::FileDownloadComplete, complete);
            }
            break;
        }
        default:
            sendMessage(MessageType::ErrorResponse, QJsonObject{{"errorCode", 400}, {"errorMessage", QStringLiteral("Unsupported message type")}});
            break;
        }
    });
    connect(m_parser, &StreamFrameParser::protocolError, this, [this](const QString& msg) {
        qWarning() << "[ Handler ] 协议错误:" << msg << ", 断开连接";
        if (m_socket) m_socket->disconnectFromHost();
    });
    if (!connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected)) {
        Log::error("ClientHandler", "Failed to connect QTcpSocket::disconnected to ClientHandler::onDisconnected");
    }
}

void ClientHandler::sendMessage(MessageType type, const QJsonObject& obj)
{
    if (!m_socket)
        return;
    QByteArray payload = toJsonPayload(obj);
    QByteArray data = pack(type, payload);
    // qInfo() << "[ Handler ] 发送消息 type=" << (quint16)type << ", 总字节=" << data.size();
    m_socket->write(data);
}

void ClientHandler::sendMessage(MessageType type)
{
    if (!m_socket) return;
    QByteArray data = pack(type, QByteArray());
    m_socket->write(data);
}

void ClientHandler::sendBinary(MessageType type, const QByteArray& bytes)
{
    if (!m_socket) return;
    QByteArray frame = pack(type, bytes);
    m_socket->write(frame);
}

void ClientHandler::onJsonResponseReady(const QJsonObject& obj)
{
    sendMessage(MessageType::JsonResponse, obj);
}

void ClientHandler::onReadyRead()
{
    if (!m_socket)
        return;
    QByteArray chunk = m_socket->readAll();
    if (!chunk.isEmpty()) m_parser->append(chunk);
}

void ClientHandler::onDisconnected()
{
    // 通知路由层清理该连接相关路由，改为通过 destroyed 连接或专用 JSON
    // 这里不再发出非 JSON 的 ClientDisconnect，交由 Router 的对象销毁清理逻辑处理

    qInfo() << "[ Handler ] 客户端断开，准备销毁自身";
    this->deleteLater();
}
