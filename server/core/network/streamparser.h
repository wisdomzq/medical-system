#pragma once

#include <QObject>
#include <QByteArray>
#include <QDataStream>
#include "core/network/protocol.h"

// 增量流解析器：将输入字节流按协议拆分为完整帧（服务端复用）
class StreamFrameParser : public QObject {
    Q_OBJECT
public:
    explicit StreamFrameParser(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    // 追加字节并尽可能多地解析完整帧
    void append(const QByteArray& data);
    void reset();

signals:
    void frameReady(Protocol::Header header, QByteArray payload);
    void protocolError(const QString& message);

private:
    QByteArray m_buffer;
};
