#include "core/network/filetransferprocessor.h"
#include <QDir>
#include <QFileInfo>

FileTransferProcessor::FileTransferProcessor(const QString& baseDir)
    : m_baseDir(baseDir)
{
}

FileTransferProcessor::~FileTransferProcessor()
{
    if (m_uploadFile.isOpen()) m_uploadFile.close();
}

void FileTransferProcessor::reset()
{
    if (m_uploadFile.isOpen()) m_uploadFile.close();
    m_name.clear();
    m_expectedSize = -1;
    m_received = 0;
}

bool FileTransferProcessor::beginUpload(const QJsonObject& meta, QJsonObject& ackOrErr)
{
    reset();
    m_name = meta.value("name").toString();
    m_expectedSize = meta.value("size").toVariant().toLongLong();
    if (m_name.isEmpty() || m_expectedSize < 0) {
        ackOrErr = QJsonObject{{"code", 400}, {"message", "invalid meta"}};
        return false;
    }
    QDir().mkpath(m_baseDir);
    const QString path = QDir(m_baseDir).filePath(m_name);
    m_uploadFile.setFileName(path);
    if (!m_uploadFile.open(QIODevice::WriteOnly)) {
        ackOrErr = QJsonObject{{"code", 500}, {"message", "open failed"}};
        reset();
        return false;
    }
    ackOrErr = QJsonObject{{"file", m_name}, {"ready", true}};
    return true;
}

bool FileTransferProcessor::appendChunk(const QByteArray& data, QJsonObject& err)
{
    if (!m_uploadFile.isOpen()) {
        err = QJsonObject{{"code", 409}, {"message", "no meta"}};
        return false;
    }
    const qint64 written = m_uploadFile.write(data);
    if (written != data.size()) {
        err = QJsonObject{{"code", 500}, {"message", "write failed"}};
        return false;
    }
    m_received += written;
    return true;
}

bool FileTransferProcessor::finishUpload(QJsonObject& resultOrErr)
{
    if (!m_uploadFile.isOpen() || m_name.isEmpty()) {
        resultOrErr = QJsonObject{{"code", 409}, {"message", "no upload in progress"}};
        return false;
    }
    m_uploadFile.flush();
    m_uploadFile.close();
    if (m_expectedSize >= 0 && m_received != m_expectedSize) {
        resultOrErr = QJsonObject{{"code", 422}, {"message", "size mismatch"}};
        reset();
        return false;
    }
    resultOrErr = QJsonObject{{"uploaded", true}, {"file", m_name}, {"size", m_received}};
    reset();
    return true;
}

bool FileTransferProcessor::downloadWhole(const QJsonObject& req,
                                          const std::function<void(const QByteArray&)>& sendChunk,
                                          QJsonObject& completeOrErr)
{
    const QString name = req.value("name").toString();
    if (name.isEmpty()) {
        completeOrErr = QJsonObject{{"code", 400}, {"message", "invalid name"}};
        return false;
    }
    const QString path = QDir(m_baseDir).filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        completeOrErr = QJsonObject{{"code", 404}, {"message", "not found"}};
        return false;
    }
    while (!f.atEnd()) {
        QByteArray chunk = f.read(64 * 1024);
        sendChunk(chunk);
    }
    const qint64 size = QFileInfo(f).size();
    f.close();
    completeOrErr = QJsonObject{{"name", name}, {"size", size}};
    return true;
}
