#pragma once

#include <QByteArray>
#include <QFile>
#include <QJsonObject>
#include <QString>
#include <functional>

// 负责处理文件上传/下载的payload解析与生成（与网络发送解耦）
class FileTransferProcessor {
public:
    explicit FileTransferProcessor(const QString& baseDir = QStringLiteral("files"));
    ~FileTransferProcessor();

    // 上传流程
    bool beginUpload(const QJsonObject& meta, QJsonObject& ackOrErr); // {name, size}
    bool appendChunk(const QByteArray& data, QJsonObject& err);       // 仅在 begin 成功后调用
    bool finishUpload(QJsonObject& resultOrErr);                       // 返回 {uploaded, file, size}

    // 下载：读取整个文件并通过回调逐块“生成”payload
    bool downloadWhole(const QJsonObject& req,
                       const std::function<void(const QByteArray&)>& sendChunk,
                       QJsonObject& completeOrErr); // req={name}; complete={name,size}

    void reset();

private:
    QString m_baseDir;
    QFile m_uploadFile;
    QString m_name;
    qint64 m_expectedSize = -1;
    qint64 m_received = 0;
};
