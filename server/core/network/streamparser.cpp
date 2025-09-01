#include "core/network/streamparser.h"

using namespace Protocol;

void StreamFrameParser::append(const QByteArray& data)
{
    if (!data.isEmpty())
        m_buffer.append(data);

    // 尽量多地解析完整帧
    while (true) {
        if (m_buffer.size() < FIXED_HEADER_SIZE)
            return;

        QDataStream ds(m_buffer);
        ds.setByteOrder(QDataStream::BigEndian);
        ds.setVersion(QDataStream::Qt_5_15);

        quint32 magic;
        quint8 version;
        quint16 type;
        quint32 payloadSize;
        ds >> magic >> version >> type >> payloadSize;

        if (magic != MAGIC || version != VERSION) {
            emit protocolError(QStringLiteral("Invalid header: magic/version mismatch"));
            m_buffer.clear();
            return;
        }
        if (payloadSize > (quint32)MAX_PACKET_SIZE) {
            emit protocolError(QStringLiteral("Payload too large"));
            m_buffer.clear();
            return;
        }

        const int total = FIXED_HEADER_SIZE + static_cast<int>(payloadSize);
        if (m_buffer.size() < total)
            return; // incomplete

        QByteArray payload = m_buffer.mid(FIXED_HEADER_SIZE, payloadSize);
        m_buffer.remove(0, total);

        Header header;
        header.magic = magic;
        header.version = version;
        header.type = static_cast<MessageType>(type);
        header.payloadSize = payloadSize;
        emit frameReady(header, payload);
        // loop to parse more if available
    }
}

void StreamFrameParser::reset()
{
    m_buffer.clear();
}
