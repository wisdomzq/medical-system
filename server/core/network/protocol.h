#pragma once

#include <QDebug>
#include <QMetaType>
#include <QtCore>

namespace Protocol {

//======================== 协议总体说明（中文） ========================
// 本模块定义了客户端与服务端之间的自定义二进制协议：
// - 固定头部（大端序）：
//   magic(4) | version(1) | type(2) | payloadSize(4)
// - 变长字段：payload(payloadSize字节)
// - payload 通常为 JSON（QJsonObject）经压缩（Compact）后的字节串
// 该头部尺寸由 FIXED_HEADER_SIZE 常量给出，双方需保持一致。
//====================================================================

// 常量定义
static constexpr quint32 MAGIC = 0x1A2B3C4D;
static constexpr quint8 VERSION = 1;

// 配置项
static constexpr quint16 SERVER_PORT = 8888;
static constexpr int MAX_PACKET_SIZE = 4 * 1024 * 1024; // 4MB
static constexpr int HEARTBEAT_INTERVAL_MS = 30000; // 30s
static constexpr int HEARTBEAT_TIMEOUT_MS = 5000; // 5s

enum class MessageType : quint16 {
    JsonRequest = 1,
    JsonResponse = 2,
    ErrorResponse = 3,
    HeartbeatPing = 4,
    HeartbeatPong = 5,
    ClientDisconnect = 6,
    // 文件传输（保留 100+ 区间）
    FileUploadMeta = 100,      // payload: JSON { name, size }
    FileUploadChunk = 101,     // payload: binary chunk
    FileUploadComplete = 102,  // payload: JSON { name, size, path }
    FileDownloadRequest = 110, // payload: JSON { name }
    FileDownloadChunk = 111,   // payload: binary chunk
    FileDownloadComplete = 112,// payload: JSON { name, size }
    FileTransferError = 199    // payload: JSON { code, message }
};

// 头部结构（大端序）
struct Header {
    quint32 magic = MAGIC;
    quint8 version = VERSION;
    MessageType type = MessageType::JsonRequest;
    quint32 payloadSize = 0;
};

static constexpr int FIXED_HEADER_SIZE = sizeof(quint32) + sizeof(quint8) + sizeof(quint16) + sizeof(quint32);
static constexpr int FILE_CHUNK_SIZE = 64 * 1024; // 64KB

// 将消息打包为协议帧：固定头 + payload
inline QByteArray pack(MessageType type, const QByteArray& payload)
{
    QByteArray out;
    out.reserve(FIXED_HEADER_SIZE + payload.size());
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setVersion(QDataStream::Qt_5_15);

    ds << (quint32)MAGIC;
    ds << (quint8)VERSION;
    ds << (quint16)type;
    ds << (quint32)payload.size();
    if (!payload.isEmpty()) {
        ds.writeRawData(payload.constData(), payload.size());
    }
    return out;
}

// 将 QJsonObject 压缩为紧凑 JSON 字节串
inline QByteArray toJsonPayload(const QJsonObject& obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// 从 JSON 字节串解析为对象；若失败，返回包含错误信息的对象并打印警告日志
inline QJsonObject fromJsonPayload(const QByteArray& data)
{
    QJsonParseError err {};
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Protocol] JSON解析失败:" << err.errorString() << ", 原始字节长度=" << data.size();
        return QJsonObject {
            { "error", QStringLiteral("Invalid JSON payload") },
            { "detail", err.errorString() },
        };
    }
    return doc.object();
}

} // namespace Protocol

Q_DECLARE_METATYPE(Protocol::MessageType)
Q_DECLARE_METATYPE(Protocol::Header)
